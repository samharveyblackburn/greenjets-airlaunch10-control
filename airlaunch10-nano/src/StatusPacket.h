
#pragma once

#include <Arduino.h>
#include <Stream.h>

class StatusPacket {
public:

    // ------------------------------------------------------------------------
    // Packet Structure
    // ------------------------------------------------------------------------

    static constexpr uint8_t SIZE = 16;
    static constexpr uint8_t VERSION = 1;

    // Packet number
    uint8_t sequence = 0;

    // System state
    uint8_t state = 0;

    // Switch statuses (0 = false, 1 = true)
    uint8_t barmSwitch = 0;
    uint8_t bfireSwitch = 0;
    uint8_t bestopSwitch = 0;

    // Accumulator measurements
    uint16_t pAccumulator_target = 0;
    uint16_t pAccumulator = 0;
    uint16_t TAccumulator = 0;

    // ------------------------------------------------------------------------
    // Encode
    // ------------------------------------------------------------------------

    void encode(uint8_t* buf) const {

        // Start identifier
        buf[0] = 0xAA;
        buf[1] = 0x55;

        // Header / state
        buf[2] = VERSION;
        buf[3] = sequence;
        buf[4] = state;

        // Switch statuses
        buf[5] = barmSwitch;
        buf[6] = bfireSwitch;
        buf[7] = bestopSwitch;

        // Accumulator values
        writeU16(buf, 8,  pAccumulator_target);
        writeU16(buf, 10, pAccumulator);
        writeU16(buf, 12, TAccumulator);

        // CRC over bytes 2-13
        uint16_t crc = crc16(&buf[2], 12);
        writeU16(buf, 14, crc);
    }

    // ------------------------------------------------------------------------
    // Send
    // ------------------------------------------------------------------------

    size_t send(Stream& port) const {
        uint8_t buf[SIZE];
        encode(buf);

        return port.write(buf, SIZE);
    }

    // ------------------------------------------------------------------------
    // Decode
    // ------------------------------------------------------------------------

    bool decode(const uint8_t* buf) {

        // Validate header
        if (buf[0] != 0xAA || buf[1] != 0x55)
            return false;

        // Validate version
        if (buf[2] != VERSION)
            return false;

        // Validate CRC
        uint16_t expectedCRC = readU16(buf, 14);
        uint16_t actualCRC = crc16(&buf[2], 12);

        if (expectedCRC != actualCRC)
            return false;

        // Validate switch values
        if (buf[5] > 1 || buf[6] > 1 || buf[7] > 1)
            return false;

        // Extract packet values
        sequence = buf[3];
        state = buf[4];

        barmSwitch = buf[5];
        bfireSwitch = buf[6];
        bestopSwitch = buf[7];

        pAccumulator_target = readU16(buf, 8);
        pAccumulator = readU16(buf, 10);
        TAccumulator = readU16(buf, 12);

        return true;
    }

private:

    // ------------------------------------------------------------------------
    // 16-bit Integer Encoding / Decoding (Little Endian)
    // ------------------------------------------------------------------------

    static void writeU16(
        uint8_t* buf,
        uint8_t offset,
        uint16_t value
    ) {
        buf[offset] = value & 0xFF;
        buf[offset + 1] = (value >> 8) & 0xFF;
    }

    static uint16_t readU16(
        const uint8_t* buf,
        uint8_t offset
    ) {
        return (uint16_t)buf[offset] |
               ((uint16_t)buf[offset + 1] << 8);
    }

    // ------------------------------------------------------------------------
    // CRC-16/CCITT-FALSE
    // ------------------------------------------------------------------------

    static uint16_t crc16(
        const uint8_t* data,
        size_t length
    ) {
        uint16_t crc = 0xFFFF;

        for (size_t i = 0; i < length; i++) {

            crc ^= (uint16_t)data[i] << 8;

            for (uint8_t bit = 0; bit < 8; bit++) {

                if (crc & 0x8000) {
                    crc = (crc << 1) ^ 0x1021;
                } else {
                    crc <<= 1;
                }
            }
        }

        return crc;
    }
};
