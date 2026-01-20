#ifndef Esp32Bridge
#define Esp32Bridge

#include <Arduino.h>

uint8_t packet[64];
uint8_t data_length;

void setupBridge(int TX_PIN, int RX_PIN, int DATA_LENGHT) {
  for (int i = 0; i < 64; i++){
    packet[i] = 0;
  }

  data_length = DATA_LENGHT;

  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  Serial.begin(115200);

  Serial.print("[Bridge] Running bridge with pin RX: ");
  Serial.print(RX_PIN);
  Serial.print(", TX: ");
  Serial.println(TX_PIN);
  Serial.println("Packet formating:");
  
  int i = 0;
  
  for(int n = 0; n < 2; n++){
    Serial.print(i);
    Serial.println(": header (255)");
    i++;
  }
  
  for(int n = 0; n < data_length; n++){
    Serial.print(i);
    Serial.println(": data");
    i++;
  }

  for(int n = 0; n < 1; n++){
    Serial.print(i);
    Serial.println(": corruption check sum");
    i++;
  }
}


void setPacketData(uint8_t data, uint8_t index) {
  packet[index] = data;
}


void tickBridge(){
  Serial2.write(255);
  Serial2.write(255);
  
  uint8_t sum = 0;
  
  for (int i = 0; i < data_length; i++){
    uint8_t byte = packet[i];
    Serial2.write(byte);
    sum += byte;
  }

  Serial2.write(sum);
}

#endif
