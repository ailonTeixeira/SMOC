#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h> // Para pinMode, digitalWrite, ESP.restart()

// --- CONFIGURAÇÕES DO PROJETO ---

// Wi-Fi
const char* ssid = "WIFI_SSID";         // SSID DA REDE WI-FI
const char* password = "SENHA_WIFI";     // SENHA DA REDE WI-FI

// MQTT
const char* mqtt_server = "BROKER_MQTT_IP"; //IP DO BROKER MQTT
const int mqtt_port = 1883;                 // Porta padrão do MQTT
const char* mqtt_topic = "smoc/pressao";    // Tópico MQTT para assinatura
const char* mqtt_client_id = "ESP32_SMOC_Actuator"; // ID único para este cliente MQTT

// --- ARQUITETURA DE HARDWARE ---
#define NUM_COMPRESSORS 4
// Mapeia os pinos GPIO para cada relé de compressor
const int RELAY_PINS[NUM_COMPRESSORS] = {2, 4, 5, 18};

// --- LÓGICA DE CONTROLE INTELIGENTE ---
#define PRESSAO_LIGA_COMPRESSOR    7.0  
#define PRESSAO_DESLIGA_COMPRESSOR 9.0  

// --- CONFIGURAÇÕES DE TEMPO REAL ---
#define CONTROL_TASK_PRIORITY 2
#define MQTT_TASK_PRIORITY    1

// --- CONFIGURAÇÕES DE REDE ---
#define MQTT_KEEPALIVE 60           
#define MQTT_SOCKET_TIMEOUT 30      
#define MAX_RECONNECT_DELAY 30000   
#define INITIAL_RECONNECT_DELAY 1000 
#define MAX_MQTT_RECONNECT_ATTEMPTS 15 
#define WIFI_CHECK_INTERVAL 10000   

// --- OBJETOS E VARIÁVEIS GLOBAIS ---
WiFiClient espClient;
PubSubClient client(espClient);
QueueHandle_t pressureDataQueue;

// Variáveis para gerenciar o rodízio e o tempo de operação dos compressores
int nextCompressorToTurnOn = 0; // Índice do próximo compressor a ser ligado
unsigned long compressorOnTime[NUM_COMPRESSORS] = {0}; // Armazena quando cada compressor foi ligado

// --- DECLARAÇÃO DAS TAREFAS E CALLBACKS ---
void taskControlLogic(void *parameter);
void taskMqttHandler(void *parameter);
void mqttCallback(char* topic, byte* payload, unsigned int length);
bool reconnectMQTT(int &reconnectAttempts);

void setup() {
  Serial.begin(115200);
  delay(100);

  // Configura todos os pinos de relé como saída e os desliga
  Serial.println("Configurando pinos dos compressores...");
  for (int i = 0; i < NUM_COMPRESSORS; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);
  }

  // Conexão Wi-Fi
  Serial.print("Atuador conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  
  unsigned long wifiTimeout = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiTimeout) < 15000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado ao Wi-Fi!");
    Serial.printf("IP: %s, RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else {
    Serial.println("\nFalha na conexão WiFi! Reiniciando...");
    delay(1000);
    ESP.restart(); // Reinicia se a conexão WiFi falhar no setup
  }

  // Configuração MQTT com timeouts
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
  client.setKeepAlive(MQTT_KEEPALIVE);
  client.setSocketTimeout(MQTT_SOCKET_TIMEOUT);

  // Criação da Fila
  pressureDataQueue = xQueueCreate(5, sizeof(float));
  if (pressureDataQueue == NULL) {
    Serial.println("Erro ao criar a fila! Reiniciando...");
    delay(1000);
    ESP.restart(); // Reinicia se a fila não puder ser criada
  }

  // Criação das Tarefas
  xTaskCreatePinnedToCore(
      taskControlLogic,     // Função da tarefa
      "Control Task",       // Nome
      4096,                 // Tamanho da pilha
      NULL,                 // Parâmetros
      CONTROL_TASK_PRIORITY,// Prioridade
      NULL,                 // Handle
      1);                   // Núcleo (Core) 1

  xTaskCreatePinnedToCore(
      taskMqttHandler,      // Função da tarefa
      "MQTT Task",          // Nome
      4096,                 // Tamanho da pilha
      NULL,                 // Parâmetros
      MQTT_TASK_PRIORITY,   // Prioridade
      NULL,                 // Handle
      0);                   // Núcleo (Core) 0

  Serial.println("Sistema de controle de 4 compressores iniciado.");
}

// ============================= CALLBACK DO MQTT ==================================== //

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  
  char payloadBuffer[length + 1];
  memcpy(payloadBuffer, payload, length);
  payloadBuffer[length] = '\0'; 

  float pressure = atof(payloadBuffer);
  
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(pressureDataQueue, &pressure, &xHigherPriorityTaskWoken);
  
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}


// =============================== FUNÇÃO DE RECONEXÃO MQTT ROBUSTA ================================== //
bool reconnectMQTT(int &reconnectAttempts) {
  // Tenta conectar ao MQTT
  if (client.connect(mqtt_client_id)) {
    if (client.subscribe(mqtt_topic)) {
      reconnectAttempts = 0; // Reset do contador
      return true;
    } else {
      return false; // Falha na inscrição
    }
  } else {
    reconnectAttempts = min(reconnectAttempts + 1, (int)MAX_MQTT_RECONNECT_ATTEMPTS);
    return false; // Falha na conexão
  }
}

// ===============================TAREFA DE REDE/MQTT (BAIXA PRIORIDADE)================================== //

void taskMqttHandler(void *parameter) {
  int reconnectAttempts = 0;
  unsigned long lastWifiCheck = 0;
  
  for (;;) {
    unsigned long currentTime = millis();
    
    // Verifica periodicamente o estado do WiFi
    if (currentTime - lastWifiCheck > WIFI_CHECK_INTERVAL) {
      if (WiFi.status() != WL_CONNECTED) {
        WiFi.reconnect();
        
        unsigned long wifiReconnectTimeout = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - wifiReconnectTimeout) < 10000) {
          vTaskDelay(pdMS_TO_TICKS(500));
        }
        
        if (WiFi.status() == WL_CONNECTED) {
          reconnectAttempts = 0; // Reset das tentativas MQTT se WiFi reconectou
        } else {
          ESP.restart(); // Reinicia o ESP32 como watchdog de WiFi
        }
      }
      lastWifiCheck = currentTime;
    }
    
    // Gestão da conexão MQTT
    if (!client.connected()) {
      if (WiFi.status() == WL_CONNECTED) {
        if (!reconnectMQTT(reconnectAttempts)) {
          // Se falhou na reconexão MQTT, verifica se atingiu o limite de tentativas
          if (reconnectAttempts >= MAX_MQTT_RECONNECT_ATTEMPTS) {
            ESP.restart(); // Reinicia o ESP32 se muitas tentativas falharem
          }
          // Calcula o delay com backoff exponencial
          int delayTime = min(INITIAL_RECONNECT_DELAY * (1 << min(reconnectAttempts, 10)), MAX_RECONNECT_DELAY);
          vTaskDelay(pdMS_TO_TICKS(delayTime));
          continue; // Volta ao início do loop
        }
      } else {
        // WiFi não conectado, espera um pouco e tenta novamente
        vTaskDelay(pdMS_TO_TICKS(5000));
        continue;
      }
    }
    
    // Processa mensagens MQTT
    client.loop();
    vTaskDelay(pdMS_TO_TICKS(MQTT_TASK_LOOP_DELAY_MS));
  }
}

// ===============================TAREFA DE CONTROLE (ALTA PRIORIDADE)================================== //
void taskControlLogic(void *parameter) {
  float receivedPressure;

  for (;;) {
    // Bloqueia aguardando dados da fila, sem consumir CPU.
    if (xQueueReceive(pressureDataQueue, &receivedPressure, portMAX_DELAY) == pdTRUE) {
      // --- LÓGICA DE CONTROLE PARA 4 COMPRESSORES ---
      
      // CONDIÇÃO PARA LIGAR UM COMPRESSOR
      if (receivedPressure <= PRESSAO_LIGA_COMPRESSOR) {
        bool compressorLigado = false;
        for (int i = 0; i < NUM_COMPRESSORS; i++) {
          int compressorIndex = (nextCompressorToTurnOn + i) % NUM_COMPRESSORS;
          if (digitalRead(RELAY_PINS[compressorIndex]) == LOW) {
            digitalWrite(RELAY_PINS[compressorIndex], HIGH);
            compressorOnTime[compressorIndex] = millis(); // Registra o tempo que foi ligado
            nextCompressorToTurnOn = (compressorIndex + 1) % NUM_COMPRESSORS;
            compressorLigado = true;
            break; // Liga apenas um por ciclo de decisão
          }
        }
      } 
      // CONDIÇÃO PARA DESLIGAR UM COMPRESSOR
      else if (receivedPressure >= PRESSAO_DESLIGA_COMPRESSOR) {
        int compressorToTurnOff = -1;
        unsigned long longestOnTime = 0;

        for (int i = 0; i < NUM_COMPRESSORS; i++) {
          if (digitalRead(RELAY_PINS[i]) == HIGH) {
            unsigned long onDuration = millis() - compressorOnTime[i];
            if (onDuration > longestOnTime) {
              longestOnTime = onDuration;
              compressorToTurnOff = i;
            }
          }
        }

        if (compressorToTurnOff != -1) {
          digitalWrite(RELAY_PINS[compressorToTurnOff], LOW);
          compressorOnTime[compressorToTurnOff] = 0; // Reseta o tempo
        }
      }
      // FAIXA DE HISTERESE: Nenhuma ação é tomada
    }
  }
}

void loop() {
  // O FreeRTOS gerencia a execução de todas as tarefas de forma preemptiva.
}
