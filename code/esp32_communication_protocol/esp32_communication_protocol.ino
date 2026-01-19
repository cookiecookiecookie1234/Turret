#include <Esp32Bridge.h>

#define TX_PIN 14
#define RX_PIN 15



void setup() {
  setupBridge(TX_PIN, RX_PIN, 16);
}



void loop() {
  setPacketData(42, 0);
  setPacketData(64, 1);
  setPacketData(77, 2);

  tickBridge();

  // Do other stuff here - won't block
  delay(100);
}