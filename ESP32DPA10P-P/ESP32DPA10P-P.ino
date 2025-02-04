#include <Arduino.h>

const int sensorPin = 34; 
const float maxVoltage = 3.3; 
const float minPressure = 0.02; 
const float maxPressure = 10; 
const int adcResolution = 4095; 
const float offsetVoltage = 0.90; // Offset de 0 bar em volts

// Função para leitura média do sensor
float readSensorValue(int pin, int samples = 50) {
  int total = 0;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
    delay(10); 
  }
  return total / (float)samples;
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
}

void loop() {
  int rawValue = readSensorValue(sensorPin); 
  float voltage = ((rawValue * maxVoltage) / adcResolution) + 0.16; 

  // Ajusta para o deslocamento (0.14V -> 0 bar)
  float pressure = (((voltage - offsetVoltage) * (maxPressure - minPressure) / (maxVoltage - offsetVoltage)) + minPressure)+ 0.26;

  // Exibe os valores na serial
  Serial.print("ADC Value: ");
  Serial.print(rawValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 2); 
  Serial.print("V | Pressure: ");
  Serial.print(pressure, 2); 
  Serial.println(" bar");

  delay(1000); 
}
