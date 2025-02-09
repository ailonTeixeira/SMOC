#include <WiFi.h>
#include <LoRa.h>
#include <PubSubClient.h>

// Configurações do Wi-Fi
const char* ssid = "ESP32_LoRa_AP"; // Nome da rede Wi-Fi 
const char* password = "12345678";  // Senha da rede Wi-Fi

// Configurações do MQTT
const char* mqttServer = "IP_DO_RASPBERRY_PI";
const int mqttPort = 1883;
const char* mqttUser = "USUARIO_MQTT";
const char* mqttPassword = "SENHA_MQTT";

// Configurações do LoRa
#define LORA_SS 18
#define LORA_RST 14
#define LORA_DIO0 26

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  // Conecta ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado ao Wi-Fi");

  // Inicializa o LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(915E6)) { // Frequência de 915 MHz (Frequência aprovada pela anatel)
    Serial.println("Falha ao iniciar o LoRa!");
    while (1);
  }
  Serial.println("LoRa iniciado");

  // Configura o cliente MQTT
  client.setServer(mqttServer, mqttPort);
}

void loop() {
  // Verifica se há dados recebidos via LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String data = "";
    while (LoRa.available()) {
      data += (char)LoRa.read();
    }
    Serial.println("Dados recebidos via LoRa: " + data);

    // Publica os dados no broker MQTT
    if (client.connect("ESP32LoRaTransmissor", mqttUser, mqttPassword)) {
      client.publish("sensor/corrente", data.c_str());
      Serial.println("Dados publicados no MQTT: " + data);
    } else {
      Serial.println("Falha na conexão com o broker MQTT");
    }
  }

  // Mantém a conexão MQTT ativa
  client.loop();
}
