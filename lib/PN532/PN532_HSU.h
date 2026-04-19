#ifndef __PN532_HSU_H__
#define __PN532_HSU_H__

#include <Arduino.h>
#include "PN532Interface.h"

class PN532_HSU : public PN532Interface {
public:
    PN532_HSU(HardwareSerial& serial) : _serial(serial) {}

    void begin() override {
        // Serial already started with pins by the caller before constructing
    }

    void wakeup() override {
        // Send wakeup preamble — pulls PN532 out of low-power mode
        _serial.write((uint8_t)0x55);
        _serial.write((uint8_t)0x55);
        _serial.write((uint8_t)0x00);
        _serial.write((uint8_t)0x00);
        _serial.write((uint8_t)0x00);
        _serial.write((uint8_t)0x00);
        _serial.write((uint8_t)0x00);
        _serial.write((uint8_t)0x00);
        _serial.write((uint8_t)0x00);
        _serial.write((uint8_t)0x00);
        _serial.write((uint8_t)0x00);
        _serial.write((uint8_t)0x00);
        _serial.write((uint8_t)0x00);
        _serial.write((uint8_t)0x00);
        _serial.write((uint8_t)0x00);
        delay(10);
    }

    int8_t writeCommand(const uint8_t* header, uint8_t hlen,
                        const uint8_t* body = nullptr, uint8_t blen = 0) override;

    int16_t readResponse(uint8_t buf[], uint8_t len, uint16_t timeout = 1000) override;

private:
    HardwareSerial& _serial;

    bool     _readByte(uint8_t* b, uint16_t timeout);
    void     _writeFrame(const uint8_t* data, uint8_t len);
    bool     _readAck(uint16_t timeout = 1000);
    void     _flush();

    static const uint8_t ACK_FRAME[6];
};

#endif // __PN532_HSU_H__
