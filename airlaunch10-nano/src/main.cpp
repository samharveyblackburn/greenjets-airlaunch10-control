
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "StatusPacket.h"

// ------------------------------------------------------------------------------------------------
// Declare variables
// ------------------------------------------------------------------------------------------------

// RX = D2, TX = D3
SoftwareSerial rs485(2, 3);

// Packet receiver
StatusPacket packet;
uint8_t rxBuffer[StatusPacket::SIZE];
uint8_t rxIndex = 0;

// ------------------------------------------------------------------------------------------------
// Print Packet
// ------------------------------------------------------------------------------------------------
void printRawPacket(const uint8_t* buf) {

    Serial.print(F("RAW: "));

    for (uint8_t i = 0; i < StatusPacket::SIZE; i++) {

        if (buf[i] < 0x10) {
            Serial.print('0');
        }

        Serial.print(buf[i], HEX);
        Serial.print(' ');
    }

    Serial.println();
}

void printPacket(const StatusPacket& p) {

    Serial.println();
    Serial.println(F("===================================="));
    Serial.println(F("          STATUS PACKET"));
    Serial.println(F("===================================="));

    // Header
    Serial.print(F("Sequence:             "));
    Serial.println(p.sequence);

    Serial.print(F("System State:         "));
    Serial.println(p.state);

    // Switches
    Serial.println(F("------------------------------------"));
    Serial.println(F("SWITCH STATUSES"));

    Serial.print(F("ARM Switch:           "));
    Serial.println(p.barmSwitch ? F("ON") : F("OFF"));

    Serial.print(F("FIRE Switch:          "));
    Serial.println(p.bfireSwitch ? F("ON") : F("OFF"));

    Serial.print(F("ESTOP Switch:         "));
    Serial.println(p.bestopSwitch ? F("ON") : F("OFF"));

    // Accumulator measurements
    Serial.println(F("------------------------------------"));
    Serial.println(F("ACCUMULATOR"));

    Serial.print(F("Target Pressure:      "));
    Serial.println(p.pAccumulator_target);

    Serial.print(F("Actual Pressure:      "));
    Serial.println(p.pAccumulator);

    Serial.print(F("Temperature:          "));
    Serial.println(p.TAccumulator);

    Serial.println(F("===================================="));
}

// ------------------------------------------------------------------------------------------------
// Process Incoming Byte
// ------------------------------------------------------------------------------------------------

void processByte(uint8_t b) {

    // Wait for first header byte
    if (rxIndex == 0) {
        if (b == 0xAA) {
            rxBuffer[rxIndex++] = b;
        }
        return;
    }

    // Wait for second header byte
    if (rxIndex == 1) {
        if (b == 0x55) {
            rxBuffer[rxIndex++] = b;
        } else if (b == 0xAA) {
            // Could be the beginning of another header
            rxBuffer[0] = b;
        } else {
            rxIndex = 0;
        }
        return;
    }

    // Store remaining packet bytes
    rxBuffer[rxIndex++] = b;

    // Complete packet received
    if (rxIndex == StatusPacket::SIZE) {

        printRawPacket(rxBuffer);

        if (packet.decode(rxBuffer)) {
            printPacket(packet);
        } else {
            Serial.println(F("WARNING: Invalid packet"));
        }

        rxIndex = 0;
    }
}

// ------------------------------------------------------------------------------------------------
// Setup
// ------------------------------------------------------------------------------------------------

void setup() {

    Serial.begin(115200);
    rs485.begin(9600);

    Serial.println(F("Nano RS485 receiver ready"));
    Serial.println(F("Waiting for status packets..."));
}

// ------------------------------------------------------------------------------------------------
// Main Loop
// ------------------------------------------------------------------------------------------------

void loop() {

    while (rs485.available()) {
        uint8_t b = rs485.read();
        processByte(b);
    }
}
