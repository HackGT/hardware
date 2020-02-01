#include <SPI.h>
#include <RFM69registers.h>
#include <RFM69.h>
#include <Ethernet.h>
#include "sha256.h"

#define ENCRYPT_KEY "HackGT 6: Rabbit"  // Must be 16 bytes long
#define BASE_STATION_ID 0
#define NETWORK_ID 6  // HackGT 6 haha

RFM69 radio(53, 21);
// Ryan Petschek's USB Ethernet adapter
// Used so as not to conflict on GT network
byte mac[] = {
  0x00, 0x50, 0xB6, 0x1E, 0x26, 0x04
};

EthernetClient client;
#define API_KEY "ZyWzJDCaFybnjcFvGaTPadqM"
#define API_KEY_LENGTH 24
//HttpClient client = HttpClient(ethernetClient, "insight.http.hack.gt", 80);
//HttpClient client = HttpClient(ethernetClient, "128.61.40.201", 3000);
Sha256 hasher;

unsigned long lastTime = 0; // Seconds since UNIX epoch
unsigned long lastUpdate = 0; // Milliseconds since Arduino startup
unsigned long getTime() {
  if (lastUpdate == 0 || (millis() - lastUpdate) > (60000 * 5)) {
    byte connectResult = client.connect("insight.http.hack.gt", 80);
    if (connectResult) {
      // Make a HTTP request:
      client.println("GET /api/scanner/time HTTP/1.1");
      client.println("Host: insight.http.hack.gt");
      client.println("Connection: close");
      client.println();
    }
    else {
      Serial.print("Error connecting: ");
      Serial.println(connectResult);
    }
    char body[12];
    byte bodyIndex = 0;
    
    while (client.connected()) {
      char lastCharacters[4] = { 0, 0, 0, 0 };
      bool isBody = false;
      while (client.available()) {
          if (lastCharacters[0] == '\r' && lastCharacters[1] == '\n' && lastCharacters[2] == '\r' && lastCharacters[3] == '\n') {
            isBody = true;
          }
          char currentChar = client.read();
          if (isBody) {
            body[bodyIndex] = currentChar;
            bodyIndex++;
          }
          lastCharacters[0] = lastCharacters[1];
          lastCharacters[1] = lastCharacters[2];
          lastCharacters[2] = lastCharacters[3];
          lastCharacters[3] = currentChar;
      }
    }
    body[bodyIndex] = 0; // NULL terminate the string
    lastTime = atol(body);
    
    lastUpdate = millis();
  }
  return ((millis() - lastUpdate) / 1000) + lastTime;
}

void setup() {
	Serial.begin(115200);
  Serial.println("Base node starting up...");

	// Initialize the RFM69HCW
	if (!radio.initialize(RF69_915MHZ, BASE_STATION_ID, NETWORK_ID)) {
		Serial.println(F("Couldn't init()"));
	}
	// Default baud rate is 55,555 bps
	// Set to 9600 bps for better range
	// This is sufficient for 30 40-byte messages per second
	radio.writeReg(REG_BITRATEMSB, RF_BITRATEMSB_9600);
	radio.writeReg(REG_BITRATELSB, RF_BITRATELSB_9600);
	radio.setHighPower();
	radio.encrypt(ENCRYPT_KEY);

	Ethernet.init(10);
	bool ethernetInitSuccess = Ethernet.begin(mac);
	if (!ethernetInitSuccess) {
		Serial.println(F("Failed to configure Ethernet using DHCP"));
		Serial.println(Ethernet.hardwareStatus());
	}
	Serial.print("Connected via DHCP\nAssigned IP: ");
  Serial.println(Ethernet.localIP());
	// Give the Ethernet shield a second to initialize
	delay(1000);

  Serial.print("Setting time...");
  // Getting the time takes a second to read body (TODO: why?)
  // so let's do it during the set up
  Serial.println(getTime());

  Serial.print(F("Base station with ID "));
  Serial.print(BASE_STATION_ID, DEC);
  Serial.print(F(" ready (took "));
  Serial.print(millis() / 1000);
  Serial.println(" seconds )");
}

enum MessageType {
	Heartbeat,
	BatteryLevel,
	Scan,
  PuzzleCompletion,
};
typedef struct {
	enum MessageType type;
	byte payload[40]; // Max 40 byte payload (UUID size + 4 bytes data)
} Message;
Message message;

void loop() {
  if (radio.receiveDone()) {
		int16_t rssi = radio.RSSI;
		uint16_t sender = radio.SENDERID;
		message = *(Message*)radio.DATA;
		radio.sendACK();

    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.print(" | Node ");
    Serial.print(sender);
    Serial.print(": ");

		if (message.type == Heartbeat) {
			Serial.print("switched on");
      Serial.println();
      sendRequest("/api/scanner/heartbeat", String(sender));
		}
		else if (message.type == BatteryLevel) {
			uint16_t voltage = message.payload[0];
			voltage += message.payload[1] << 8;
      uint8_t percentage = asigmoidal(voltage, 3400, 4200);

			Serial.print("battery at ");
			Serial.print(percentage);
			Serial.print("% (");
			Serial.print(voltage);
			Serial.print(" mV)");
      Serial.println();
      sendRequest("/api/scanner/battery", String(sender) + "|" + String(voltage) + "|" + String(percentage));
		}
		else if (message.type == Scan) {
      String uuid = "";
			for (byte i = 0; i < 36; i++) {
				uuid += (char) message.payload[i];
			}
      Serial.println(uuid);
      sendRequest("/api/scanner/visit", String(sender) + "|" + uuid);
		}
   else if (message.type == PuzzleCompletion) {
      byte puzzle = message.payload[0];
      String slug;
      String answer;
      if (puzzle == 0) {
        slug = "lobster-beach-stage-2";
        answer = "ddr done";
      }
      else if (puzzle == 1) {
        slug = "button-wall-stage-4";
        answer = "button wall done";
      }

      String uuid = "";
      for (byte i = 0; i < 36; i++) {
        // UUID starts at 2nd byte
        uuid += (char) message.payload[i + 1];
      }
      sendRequest("/api/scanner/puzzle", uuid + "|" + slug + "|" + answer);
   }
	}
}

void sendRequest(char* url, String postData) {
  client.stop();
  unsigned long currentTime = getTime();
  
  String signature = toHexString(signRequest(currentTime, postData.c_str(), postData.length()), 32);
  String authorizedBody = signature + " " + currentTime + "\n" + postData;

  byte connectResult = client.connect("insight.http.hack.gt", 80);
//  byte connectResult = client.connect("192.168.1.15", 3000);
  if (connectResult) {
    // Make a HTTP request:
    client.print("POST ");
    client.print(url);
    client.println(" HTTP/1.1");
    client.println("Host: insight.http.hack.gt");
//    client.println("Host: localhost");
    client.println("Content-Type: text/plain");
    client.print("Content-Length: ");
    client.println(authorizedBody.length());
    client.println("Connection: close");
    client.println();
    client.println(authorizedBody);
    client.println();
  }
  else {
    Serial.print("Error connecting: ");
    Serial.println(connectResult);
  }
}

uint8_t* signRequest(unsigned long currentTime, uint8_t* data, uint16_t dataLength) {
  String key = String(API_KEY) + currentTime;
  hasher.initHmac(key.c_str(), key.length());
  for (uint16_t i = 0; i < dataLength; i++) {
    hasher.write(data[i]);
  }
  return hasher.resultHmac();
}

char hexCharacters[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

String toHexString(uint8_t* data, uint16_t dataLength) {
  String hex = "";
  hex.reserve(64);
  for (uint16_t i = 0; i < dataLength; i++) {
    hex += hexCharacters[data[i] >> 4];
    hex += hexCharacters[data[i] & 0xf];
  }
  return hex;
}

// Asigmoidal battery percentage estimation
// Sourced from https://github.com/rlogiacco/BatterySense
uint8_t asigmoidal(uint16_t voltage, uint16_t minVoltage, uint16_t maxVoltage) {
	uint8_t result = 101 - (101 / pow(1 + pow(1.33 * (voltage - minVoltage) / (maxVoltage - minVoltage), 4.5), 3));
	return result >= 100 ? 100 : result;
}
