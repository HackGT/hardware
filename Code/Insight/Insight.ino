#include <SPI.h>
#include <RFM69registers.h>
#include <RFM69.h>
#include "MFRC522.h"

#define RED 12
#define GREEN 13
#define BUZZER 11
#define BAT_REF A7
#define NFC_RST_PIN 10
#define NFC_SS_PIN 9
#define RF_SS_PIN 8
#define RF_IRQ_PIN 14

#define ENCRYPT_KEY "HackGT 6: Rabbit" // Must be 16 bytes long
#define BASE_STATION_ID 0
#define NETWORK_ID 6 // HackGT 6 haha
#define RETRY_COUNT 3
#define ACK_TIMEOUT 50 // ms

// IMPORTANT: Set this to the ID of the label on the board
#define BOARD_ID 1

// Enables Serial debugging output
// Do not enable in production in order to reduce compiled output size
//#define DEBUG

MFRC522 mfrc522(NFC_SS_PIN, NFC_RST_PIN);
RFM69 radio(RF_SS_PIN, RF_IRQ_PIN);

enum MessageType {
	Heartbeat,
	BatteryLevel,
	Scan,
  PuzzleCompletion
};
typedef struct {
	enum MessageType type;
	byte payload[36]; // Max 36 byte payload (UUID size)
} Message;

void setup() {
    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    pinMode(BAT_REF, INPUT);
    pinMode(NFC_RST_PIN, OUTPUT);
    pinMode(NFC_SS_PIN, OUTPUT);

	#ifdef DEBUG
    	Serial.begin(9600);
		Serial.print(F("Board ID is: "));
		Serial.println(BOARD_ID);
	#endif

  SPI.begin();
  mfrc522.PCD_Init();
  delay(50);
//     mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
	#ifdef DEBUG
    	mfrc522.PCD_DumpVersionToSerial();
	#endif

	if (!radio.initialize(RF69_915MHZ, BOARD_ID, NETWORK_ID)) {
		#ifdef DEBUG
			Serial.println(F("Couldn't init() radio"));
		#endif
	}
	// Default baud rate is 55,555 bps
	// Set to 9600 bps for better range
	// This is sufficient for 30 40-byte messages per second
	radio.writeReg(REG_BITRATEMSB, RF_BITRATEMSB_9600);
	radio.writeReg(REG_BITRATELSB, RF_BITRATELSB_9600);
	radio.setHighPower();
	radio.encrypt(ENCRYPT_KEY);

	digitalWrite(GREEN, HIGH);
	digitalWrite(RED, HIGH);
	tone(BUZZER, 300);

	Message message;
	message.type = Heartbeat;
	bool sendSuccess = radio.sendWithRetry(BASE_STATION_ID, (const void*)(&message), sizeof(Message), RETRY_COUNT, ACK_TIMEOUT);
	if (sendSuccess) {
		tone(BUZZER, 1000);
	}
	else {
		tone(BUZZER, 200);
		digitalWrite(GREEN, LOW);
	}
	delay(150);
	noTone(BUZZER);
	digitalWrite(GREEN, LOW);
	digitalWrite(RED, LOW);
}

unsigned long lastBatteryReport = 0;
void loop() {
	radio.sleep();

	handleNFC();

	// Report battery level every ~15 seconds
	if (lastBatteryReport == 0 || millis() - lastBatteryReport >= 15000) {
		reportBatteryLevel();
		lastBatteryReport = millis();
	}
}

void reportBatteryLevel() {
	// 1.47 is the voltage divider ratio
	// 3300 is the reference voltage (3.3 volts)
	// ATtinys have a 10 bit ADC -> divide by 1024
	uint16_t voltage = analogRead(BAT_REF) * 1.47 * 3300 / 1024;

	Message message;
	message.type = BatteryLevel;
	message.payload[0] = voltage & 0xFF; // Lower byte
	message.payload[1] = voltage >> 8; // Upper byte

	#ifdef DEBUG
		Serial.print("Battery voltage: ");
		Serial.println(voltage, DEC);
	#endif

	radio.sendWithRetry(BASE_STATION_ID, (const void*)(&message), sizeof(Message), RETRY_COUNT, ACK_TIMEOUT);
}

void handleNFC() {
	if (!mfrc522.PICC_IsNewCardPresent()) {
		return;
	}
	// Selects one of the cards in the RF field
	if (!mfrc522.PICC_ReadCardSerial()) {
		return;
	}

	/*
	 * We want to read pages 0x04 to 0x14 in order to read the entire used data block off the card
	 *     This is because https://info.hack.gt?user=<uuid> is exactly long enough to end at page 0x14
	 * However, the FIFO buffer used in the MFRC522 has a max capacity of 64 bytes (62 usable after 2 byte response CRC)
	 * Therefore, we read starting at page 0x06 which is partially into the TLV and NDEF data blocks
	 * We just scan until we see the string `?user=` and assume everything after that is the UUID
	 * We stop reading when we reach a byte with value 0xFE since this is the TLV block terminator byte
	 */

	const byte startPage = 0x06;
	const byte endPage = 0x14; // Just long enough for HackGT badge IDs
	byte receiveBufferSize = 62; // Only 62 usable bytes can be returned from the MFRC522 FIFO buffer after 2 byte CRC is removed
	byte receiveBuffer[receiveBufferSize];
	byte result = mfrc522.MIFARE_FastRead(startPage, endPage, (byte*)&receiveBuffer, &receiveBufferSize);
	#ifdef DEBUG
		// hexPrint(receiveBuffer, receiveBufferSize);
	#endif

	Message message;
	message.type = Scan;

	bool isUUIDSection = false;
	const char uuidSeparator[6] = "?user=";
	for (byte i = 0, uuidIndex = 0; i < receiveBufferSize; i++) {
		if (receiveBuffer[i] == 0xFE) break; // TLV terminator
		if (!isUUIDSection
			&& (i + 5) < receiveBufferSize
			&& receiveBuffer[i + 0] == uuidSeparator[0]
			&& receiveBuffer[i + 1] == uuidSeparator[1]
			&& receiveBuffer[i + 2] == uuidSeparator[2]
			&& receiveBuffer[i + 3] == uuidSeparator[3]
			&& receiveBuffer[i + 4] == uuidSeparator[4]
			&& receiveBuffer[i + 5] == uuidSeparator[5]) {
			// UUID starts after query string in https://hack.gt?user=<uuid>
			i += 5;
			isUUIDSection = true;
			continue;
		}
		if (isUUIDSection) {
			message.payload[uuidIndex] = receiveBuffer[i];
			uuidIndex++;
		}
	}

	mfrc522.PICC_HaltA();  // Stop reading

	if (!isUUIDSection) {
		failure();
		delay(150);
		failure();

		#ifdef DEBUG
			Serial.println(F("No UUID found on tapped badge"));
		#endif
		return;
	}

	success();

	bool sendSuccess = radio.sendWithRetry(BASE_STATION_ID, (const void*)(&message), sizeof(Message), RETRY_COUNT, ACK_TIMEOUT);
	if (!sendSuccess) {
		failure();
		#ifdef DEBUG
			Serial.print(F("Got no ACK for UUID: "));
			Serial.println((char*)message.payload);
		#endif
		return;
	}

	#ifdef DEBUG
		Serial.print(F("Sent UUID: "));
		Serial.println((char*)message.payload);
	#endif
}

void success() {
	digitalWrite(GREEN, HIGH);
	tone(BUZZER, 1000);
	delay(250);
	noTone(BUZZER);
	digitalWrite(GREEN, LOW);
}
void failure() {
	digitalWrite(RED, HIGH);
	tone(BUZZER, 300);
	delay(250);
	noTone(BUZZER);
	digitalWrite(RED, LOW);
}

void hexPrint(byte* buff, byte buffSize) {
    for (byte i = 0; i < buffSize; i++) {
        if (i % 4 == 0)
            Serial.println();

        if (buff[i] < 0x10)
            Serial.print("0");
        Serial.print(buff[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}
