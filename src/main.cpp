//#define TRANSMITTER_TEST

#include <Arduino.h>
#include <RH_RF95.h>
#include <SoftwareSerial.h>

#include "config.h"

SoftwareSerial to485(TO485_RX, TO485_TX);
RH_RF95 rf95(RFM95_CS, RFM95_INT);
LORA_MODE_ENUM loraModeEnum = LORA_MODE_RECEIVER;

void blinkLedBlue(int delayTime) {
  digitalWrite(BLUE_LED, HIGH);
  delay(delayTime);
  digitalWrite(BLUE_LED, LOW);
  delay(delayTime);
}

void setLoRaMode(LORA_MODE_ENUM mode) {
  switch (mode) {
    case LORA_MODE_TRANSMITTER: {
      if (loraModeEnum != LORA_MODE_TRANSMITTER) {
        loraModeEnum = LORA_MODE_TRANSMITTER;
        digitalWrite(RS485_TRANS, HIGH); // set rs485 as transmitter
        delay(10);
      }
      break;
    }
    case LORA_MODE_RECEIVER: {
      if (loraModeEnum != LORA_MODE_RECEIVER) {
        loraModeEnum = LORA_MODE_RECEIVER;
        digitalWrite(RS485_TRANS, LOW); // set rs485 as receiver
        delay(10);
      }
      break;
    }
  }

}

void setup() {
  Serial.begin(9600);
  to485.begin(9600);
  pinMode(PWD_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  pinMode(RS485_TRANS, OUTPUT);

  // reset rf module
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    for (int i = 0; i < 8; i++) {
      blinkLedBlue(100);
    }
    delay(1000);
  }

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    for (int i = 0; i < 8; i++) {
      blinkLedBlue(100);
    }
    delay(1000);
  }

  rf95.setTxPower(23, false);
  digitalWrite(PWD_LED, HIGH); // solid pwd led
  setLoRaMode(LORA_MODE_RECEIVER);

  Serial.println("RFM95 Initialized!");
}

void loop() {
  if (rf95.available()) {
    setLoRaMode(LORA_MODE_TRANSMITTER);
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)) {
      digitalWrite(BLUE_LED, HIGH);
      Serial.print("Got: ");
      for (int i = 0; i < len; i++) {
        Serial.print(buf[i], HEX); Serial.print(" ");
      }
      Serial.println();
      to485.write(buf, len);
      delay(50);
      digitalWrite(BLUE_LED, LOW);
      setLoRaMode(LORA_MODE_RECEIVER);
    }
    else {
      blinkLedBlue(10);
    }
  }

  if (to485.available()) {
    byte raw[64];
    int readIndex = 0;
    uint32_t lastByteTs = micros();
    while (true) {
      if (to485.available()) {
        raw[readIndex] = to485.read();
        Serial.print(raw[readIndex], HEX);
        Serial.print(" ");
        readIndex++;
        lastByteTs = micros();
      }

      if (micros() - lastByteTs >= 10000) {
        Serial.println();
        Serial.print("Sending Packet...");
        setLoRaMode(LORA_MODE_TRANSMITTER);
        rf95.send(raw, readIndex);
        for (int i = 0; i < 2; i++) blinkLedBlue(30);
        rf95.waitPacketSent();
        setLoRaMode(LORA_MODE_RECEIVER);
        Serial.println("Success!");
        break;
      }
    }
  }
}