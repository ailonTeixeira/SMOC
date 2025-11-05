#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>

// --- CONFIGURAÇÕES DO PROJETO ---

// Wi-Fi
const char* ssid = "SEU_WIFI_SSID";         // SSID DA REDE WI-FI
const char* password = "SUA_SENHA_WIFI";     // SENHA DA  REDE WI-FI

// MQTT
const char* mqtt_server = "BROKER_MQTT_IP"; // IP OU DOMÍNIO DO BROKER MQTT
const int mqtt_port = 1883;                 // Porta padrão do MQTT
const char* mqtt_topic = "smoc/pressao";    // Tópico MQTT para publicação da pressão
const char* mqtt_client_id = "ESP32_SMOC_Sensor"; // ID único para este cliente MQTT

// Sensor DPA10-P
const int sensorPin = 34;                   // Pino ADC para o sensor de pressão (GPIO34)
const float maxVoltage = 3.3;               // Tensão máxima de referência do ADC 
const float minPressure = 0.02;             // Pressão mínima que o sensor pode medir (em bar)
const float maxPressure = 10;               // Pressão máxima que o sensor pode medir (em bar)
const int adcResolution = 4095;             // Resolução do ADC (12 bits -> 2^12 - 1 = 4095)
const float offsetVoltage = 0.90;           // Tensão de saída do sensor para 0 bar (em Volts)
const float voltageCorrection = 0.16;       // Correção de tensão observada
const float pressureCorrection = 0.26;      // Correção de pressão observada

// --- CONFIGURAÇÕES DE TEMPO REAL ---
#define SENSOR_READ_PERIOD_MS 1000 
#define MQTT_TASK_DELAY_MS 20      
#define MAX_MQTT_RECONNECT_ATTEMPTS 10 

// Prioridades das tarefas FreeRTOS
#define SENSOR_TASK_PRIORITY 2
#define MQTT_TASK_PRIORITY   1

// --- OBJETOS GLOBAIS ---
WiFiClient espClient;
PubSubClient client(espClient);
QueueHandle_t pressureQueue; // Fila para comunicação entre tarefas

// --- DECLARAÇÃO DAS TAREFAS (FreeRTOS) ---
void taskSensorReader(void *parameter);
void taskMqttHandler(void *parameter);

// Função para leitura média do sensor
float readSensorValue(int pin, int samples = 50) {
  unsigned long total = 0;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
    delay(1); // Pequeno delay entre amostras para estabilização
  }
  return total / (float)samples;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Configuração do ADC
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db); // ADC_11db = 0-3.3V, ajuste conforme seu circuito e sensor

  // Conecta ao Wi-Fi
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado ao Wi-Fi!");

  // Configura o servidor MQTT
  client.setServer(mqtt_server, mqtt_port);

  // Cria a fila para 5 itens do tipo float
  pressureQueue = xQueueCreate(5, sizeof(float));
  if (pressureQueue == NULL) {
    while(true); // Trava o sistema se a fila não puder ser criada
  }

  // Cria e fixa a tarefa de leitura do sensor no Core 1 (alta prioridade)
  xTaskCreatePinnedToCore(
      taskSensorReader,     // Função da tarefa
      "Sensor Reader Task", // Nome
      4096,                 // Tamanho da pilha
      NULL,                 // Parâmetros
      SENSOR_TASK_PRIORITY, // Prioridade
      NULL,                 // Handle
      1);                   // Núcleo (Core) 1

  // Cria e fixa a tarefa MQTT no Core 0 (baixa prioridade)
  xTaskCreatePinnedToCore(
      taskMqttHandler,      // Função da tarefa
      "MQTT Handler Task",  // Nome
      4096,                 // Tamanho da pilha
      NULL,                 // Parâmetros
      MQTT_TASK_PRIORITY,   // Prioridade
      NULL,                 // Handle
      0);                   // Núcleo (Core) 0

  Serial.println("Sistema FreeRTOS iniciado. Tarefas criadas.");
}

void loop() {
  //FreeRTOS gerencia a execução das tarefas.
}

// ============================== TAREFA DE LEITURA DO SENSOR =================================== //

void taskSensorReader(void *parameter) {
  for (;;) {
    float rawValue = readSensorValue(sensorPin);
    
    float voltage = ((rawValue * maxVoltage) / adcResolution) + voltageCorrection;
    float pressure = (((voltage - offsetVoltage) * (maxPressure - minPressure) / (maxVoltage - offsetVoltage)) + minPressure) + pressureCorrection;
    
    pressure = max(minPressure, min(pressure, maxPressure));

    xQueueSend(pressureQueue, &pressure, portMAX_DELAY);
    
    vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_PERIOD_MS));
  }
}

// =============================== TAREFA DE COMUNICAÇÃO MQTT ================================== //

void taskMqttHandler(void *parameter) {
  float receivedPressure;
  uint8_t reconnectAttempts = 0; // Contador de tentativas de reconexão MQTT

  for (;;) {
    if (!client.connected()) {
      reconnectAttempts++;
      if (reconnectAttempts > MAX_MQTT_RECONNECT_ATTEMPTS) {
        // Se exceder o número máximo de tentativas, reinicia o ESP32
        // Watchdog para a conexão MQTT.
        ESP.restart(); // Força um reset de software
      }

      if (!client.connect(mqtt_client_id)) {
        // Falha na reconexão, espera um pouco para tentar novamente
        vTaskDelay(pdMS_TO_TICKS(2000));
        continue; // Pula o resto do loop e tenta novamente
      } else {
        // Conectado com sucesso, reseta o contador de tentativas
        reconnectAttempts = 0;
      }
    } else {
      // Conectado, reseta o contador de tentativas
      reconnectAttempts = 0;
    }

    client.loop(); // Mantém a conexão MQTT e processa callbacks

    if (xQueueReceive(pressureQueue, &receivedPressure, 0) == pdTRUE) {
      char payload[10];
      snprintf(payload, sizeof(payload), "%.2f", receivedPressure);
      client.publish(mqtt_topic, payload);
    }
    
    vTaskDelay(pdMS_TO_TICKS(MQTT_TASK_DELAY_MS));
  }
}
