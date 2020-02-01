#include "MCP23S17.h"
#include <NRFLite.h>

MCP23S17 expander(0, 0);
NRFLite radio(Serial);

#define MAX_NODES 70

uint16_t id = 0;
void setup() {
  // Seed random number generator with random analogRead values on an unconnected pin
  randomSeed(analogRead(A7));

  id = random(65536);
  
  Serial.begin(115200);
  Serial.print("Badge ID: ");
  Serial.println(id);
  
  expander.begin();
  for (int i = 0; i < 16; i++) {
    expander.pinMode(i, OUTPUT);
  }

  if (!radio.init(0, 10, 9, NRFLite::BITRATE250KBPS)) {
    Serial.println("Radio comms failed!");
    while (true);
  }
}

// In order left to right
//int ledOrder[12] = { 10, 11, 12, 13, 9, 8, 15, 14, 4, 7, 6, 5 };
// Starting from center
int ledOrder[12] = { 8, 15, 9, 14, 13, 4, 12, 7, 11, 6, 10, 5 };

void loop() {
  uint16_t nodeList[MAX_NODES];
  short nodeListIndex = 0;
  for (short i = 0; i < MAX_NODES; i++) {
    nodeList[i] = 0;
  }
  
  uint16_t data = id;
  radio.send(0, &data, 2, NRFLite::NO_ACK);

  uint8_t foundNodes = 1; // You!
  unsigned long listenStart = millis();
  unsigned long waitTime = 1000; // + random(500); // Random interval from 500 to 999ms to prevent synchronization
  while (true) {
    if (millis() - listenStart > waitTime) {
      break;
    }
    while (radio.hasData()) {
      radio.readData(&data);
      Serial.print("Found: ");
      Serial.println(data);
      bool alreadyFound = false;
      for (short i = 0; i < nodeListIndex; i++) {
        if (nodeList[i] == data) {
          alreadyFound = true;
        }
      }
      if (!alreadyFound) {
        nodeList[nodeListIndex] = data;
        nodeListIndex++;
      }
    }
  }
  foundNodes += nodeListIndex;
  
  // Turn on for count of nodes
  Serial.print("Found ");
  Serial.print(foundNodes);
  Serial.println(" nodes");
  for (uint8_t i = 0; i < foundNodes && i < 12; i++) {
    expander.digitalWrite(ledOrder[i], LOW);
  }
  // Turn off unused LEDs
  for (uint8_t i = foundNodes; i < 12; i++) {
    expander.digitalWrite(ledOrder[i], HIGH);
  }
}
