#include <Stepper.h>
#include "RTClib.h"

RTC_DS1307 rtc;

const int stepsPerRevolution = 200;
Stepper minuteHand(stepsPerRevolution, A3, 2, 3, A2);

// Hour 0 (aka 12) through 11
int hourPins[12] = {12, 8, A1, 7, A0, 9, 13, 6, 4, 5, 11, 10};

void setup () {
  Serial.begin(115200);
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  for (int i = 0; i < 12; i++) {
    pinMode(hourPins[i], OUTPUT);
    digitalWrite(hourPins[i], LOW);
  }
  minuteHand.setSpeed(5);
}

int previousHour = -1;
// This means that the minute hand must be started in the 0 position when the clock is powered on
int minuteHandPosition = 0; // Range: 0 - 199
void loop () {
    DateTime now = rtc.now();

    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    int currentHour = now.hour() % 12;
    if (previousHour != currentHour) {
      for (int i = 0; i < 12; i++) {
        digitalWrite(hourPins[i], LOW);
      }
      digitalWrite(hourPins[currentHour], HIGH);
      previousHour = currentHour;
    }

    float minutes = (float) now.minute();
    float seconds = (float) now.second();
    const float sixty = (float) 60;
    int newPosition = ((minutes / sixty) + (seconds / sixty / sixty)) * stepsPerRevolution;
    int moveAmount = 0;
    if (newPosition < minuteHandPosition) {
      moveAmount = (newPosition + stepsPerRevolution) - minuteHandPosition;
    }
    else {
      moveAmount = newPosition - minuteHandPosition;
    }
    if (moveAmount > 0) {
      // Negate in order to move clockwise
      minuteHand.step(-moveAmount);
    }
    minuteHandPosition = newPosition;

    delay(100);
}
