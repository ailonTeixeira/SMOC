#include <WiFi.h>

// Configurações do Wi-Fi
const char* ssid = "ESP32_LoRa_AP"; // Nome da rede Wi-Fi criada pelo ESP32 LoRa (Receptor)
const char* password = "12345678";  // Senha da rede Wi-Fi

// Endereço IP do ESP32 LoRa (Receptor)
const char* serverIP = "192.168.4.1"; // IP padrão do ponto de acesso
const int serverPort = 80; // Porta do servidor

// Pino do sensor de corrente SCT-013
const int sensorPin = 34; // Pino analógico

void setup() {
  Serial.begin(115200);

  // Conecta ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado ao Wi-Fi");
}

void loop() {
  // Lê o valor do sensor de corrente
  int sensorValue = analogRead(sensorPin);
  float current = (sensorValue / 4095.0) * 100.0; // Converte para corrente (ajuste conforme necessário)

  // Envia os dados para o ESP32 LoRa (Receptor)
  WiFiClient client;
  if (client.connect(serverIP, serverPort)) {
    String data = "ESP32_1:" + String(current); // Formato: "ESP32_1:valor"
    client.println(data);
    Serial.println("Dados enviados: " + data);
    client.stop();
  } else {
    Serial.println("Falha na conexão com o ESP32 LoRa (Receptor)");
  }

  delay(5000); // Espera 5 segundos antes de enviar novamente
}
