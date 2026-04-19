#ifndef __PN532_I2C_H__
#define __PN532_I2C_H__

#include <Arduino.h>
#include <Wire.h>
#include "PN532Interface.h"

#define PN532_I2C_ADDRESS   (0x24)

class PN532_I2C : public PN532Interface {
public:
    PN532_I2C(TwoWire& wire = Wire) : _wire(wire) {}

    void begin() override {}   // Wire.begin() called by manager before constructing

    void wakeup() override {
        // Drive a dummy I2C transaction to wake PN532 from low-power state
        _wire.beginTransmission(PN532_I2C_ADDRESS);
        _wire.write((uint8_t)0x00);
        _wire.endTransmission();
        delay(2);
    }

    int8_t  writeCommand(const uint8_t* header, uint8_t hlen,
                         const uint8_t* body = nullptr, uint8_t blen = 0) override;
    int16_t readResponse(uint8_t buf[], uint8_t len, uint16_t timeout = 1000) override;

private:
    TwoWire& _wire;

    // Poll PN532 until it signals ready (first byte of I2C read = 0x01)
    bool _waitReady(uint16_t timeout);

    static const uint8_t ACK_FRAME[6];
};

#endif // __PN532_I2C_H__
