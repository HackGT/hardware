#include <SPI.h>
#include "MCP23S17.h"
#include "Adafruit_TLC5947.h"

#define NUM_TLC5947 5

#define BLANK A1
#define SIN A3
#define CLK A2
#define LATCH A0
#define oe  -1  // set to -1 to not use the enable pin (its optional)

Adafruit_TLC5947 LEDController = Adafruit_TLC5947(NUM_TLC5947, CLK, SIN, LATCH);

MCP23S17 ADCController(10, 0);
MCP23S17 BTN1(10, 1);
MCP23S17 BTN2(10, 2);
MCP23S17 BTN3(10, 3);
MCP23S17 BTN4(10, 4);
MCP23S17 BTN5(10, 5);
MCP23S17 BTN6(10, 6);
MCP23S17 BTN7(10, 7);

typedef struct {
  bool pressed;
  uint16_t pwm;
} Button;

Button buttons[13][8];

void wipeBoard(uint16_t value) {
  for (uint8_t x = 0; x < 13; x++) {
    for (uint8_t y = 0; y < 8; y++) {
      buttons[x][y].pwm = value;
    }
  }
}

void renderAndUpdate() {
  for (uint8_t x = 0; x < 13; x++) {
    for (uint8_t y = 0; y < 8; y++) {
      uint8_t index = (x * 13) + y;
      LEDController.setPWM(index, buttons[x][y].pwm);
      
      if (index % 16 == 0) {
        buttons[x][y].pressed = BTN1.digitalRead(index) == LOW;
      }
      else if (index % 16 == 1) {
        buttons[x][y].pressed = BTN2.digitalRead(index) == LOW;
      }
      else if (index % 16 == 2) {
        buttons[x][y].pressed = BTN3.digitalRead(index) == LOW;
      }
      else if (index % 16 == 3) {
        buttons[x][y].pressed = BTN4.digitalRead(index) == LOW;
      }
      else if (index % 16 == 4) {
        buttons[x][y].pressed = BTN5.digitalRead(index) == LOW;
      }
      else if (index % 16 == 5) {
        buttons[x][y].pressed = BTN6.digitalRead(index) == LOW;
      }
      else if (index % 16 == 6) {
        buttons[x][y].pressed = BTN7.digitalRead(index) == LOW;
      }
    }
  }
//  LEDController.write();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Button wall!");

  pinMode(BLANK, OUTPUT);
  digitalWrite(BLANK, HIGH);

  LEDController.begin();
  for (int i = 0; i < 104; i++) {
    LEDController.setPWM(i, 4095);
    LEDController.write();
  }
  digitalWrite(BLANK, LOW);
  
  ADCController.begin();
  BTN1.begin();
  
  for (int i = 0; i < 16; i++) {
    ADCController.pinMode(i, OUTPUT);
    ADCController.digitalWrite(i, HIGH); // CS pins are active low
    BTN1.pinMode(i, INPUT_PULLUP);
    BTN2.pinMode(i, INPUT_PULLUP);
    BTN3.pinMode(i, INPUT_PULLUP);
    BTN4.pinMode(i, INPUT_PULLUP);
    BTN5.pinMode(i, INPUT_PULLUP);
    BTN6.pinMode(i, INPUT_PULLUP);
    BTN7.pinMode(i, INPUT_PULLUP);
  }
}

short brightness = 0;
short adjustment = 40;
short sign = 1;
void loop() {
  brightness += sign * adjustment;
  if (brightness >= 4096) {
    sign = -1;
    brightness = 4095;
  }
  else if (brightness < 0) {
    sign = 1;
    brightness = 0;
  }
  for (int i = 0; i < 104; i++) {
    LEDController.setPWM(i, brightness);
  }
  LEDController.write();
  delay(5);
}
