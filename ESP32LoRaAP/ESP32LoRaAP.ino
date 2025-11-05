#include <WiFi.h>
#include <LoRa.h>

// Configurações do Wi-Fi (Ponto de Acesso)
const char* ssid = "ESP32_LoRa_AP"; // Nome da rede Wi-Fi
const char* password = "12345678";  // Senha da rede Wi-Fi

// Configurações do LoRa
#define LORA_SS 18
#define LORA_RST 14
#define LORA_DIO0 26

// Servidor Wi-Fi
WiFiServer server(80); // Porta 80 para comunicação

void setup() {
  Serial.begin(115200);

  // Configura o ESP32 como ponto de acesso
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Endereço IP do AP: ");
  Serial.println(IP);

  // Inicializa o LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(915E6)) { // Frequência de 915 MHz 
    Serial.println("Falha ao iniciar o LoRa!");
    while (1);
  }
  Serial.println("LoRa iniciado");

  // Inicia o servidor Wi-Fi
  server.begin();
}

void loop() {
  // Verifica se há clientes conectados 
  WiFiClient clientWiFi = server.available();
  if (clientWiFi) {
    float data = clientWiFi.readStringUntil('\n');
    Serial.println("Dados recebidos via Wi-Fi: " + data);

    // Transmite os dados via LoRa
    LoRa.beginPacket();
    LoRa.print(data);
    LoRa.endPacket();
    Serial.println("Dados enviados via LoRa: " + data);

    clientWiFi.stop();
  }
}

