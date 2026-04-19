#include "PN532_HSU.h"
#include "PN532Interface.h"

// ACK frame: 00 00 FF 00 FF 00
const uint8_t PN532_HSU::ACK_FRAME[6] = { 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00 };

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool PN532_HSU::_readByte(uint8_t* b, uint16_t timeout) {
    uint32_t start = millis();
    while (!_serial.available()) {
        if (millis() - start > timeout) return false;
        delay(1);
    }
    *b = _serial.read();
    return true;
}

void PN532_HSU::_flush() {
    delay(3);
    while (_serial.available()) _serial.read();
}

void PN532_HSU::_writeFrame(const uint8_t* data, uint8_t len) {
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) sum += data[i];

    _serial.write(PN532_PREAMBLE);
    _serial.write(PN532_STARTCODE1);
    _serial.write(PN532_STARTCODE2);
    _serial.write(len);
    _serial.write(~len + 1);        // LCS: length checksum
    for (int i = 0; i < len; i++) _serial.write(data[i]);
    _serial.write(~sum + 1);        // DCS: data checksum
    _serial.write(PN532_POSTAMBLE);
}

bool PN532_HSU::_readAck(uint16_t timeout) {
    uint8_t buf[6];
    for (int i = 0; i < 6; i++) {
        if (!_readByte(&buf[i], timeout)) return false;
    }
    return memcmp(buf, ACK_FRAME, 6) == 0;
}

// ---------------------------------------------------------------------------
// PN532Interface implementation
// ---------------------------------------------------------------------------

int8_t PN532_HSU::writeCommand(const uint8_t* header, uint8_t hlen,
                                const uint8_t* body,   uint8_t blen) {
    _flush();

    // Build frame data: TFI + header + body
    uint8_t frame[hlen + blen + 1];
    frame[0] = PN532_HOSTTOPN532;
    memcpy(frame + 1, header, hlen);
    if (body && blen) memcpy(frame + 1 + hlen, body, blen);
    _writeFrame(frame, hlen + blen + 1);

    // HSU ACK timeout: 100ms — PN532_ACK_WAIT_TIME (10ms) is too short for UART
    if (!_readAck(100)) {
        return PN532_INVALID_ACK;
    }
    return 0;
}

int16_t PN532_HSU::readResponse(uint8_t buf[], uint8_t len, uint16_t timeout) {
    uint8_t b;

    // Wait for preamble 0x00
    if (!_readByte(&b, timeout) || b != 0x00) return PN532_TIMEOUT;
    // Start code 0x00 0xFF
    if (!_readByte(&b, timeout) || b != 0x00) return PN532_INVALID_FRAME;
    if (!_readByte(&b, timeout) || b != 0xFF) return PN532_INVALID_FRAME;

    // Length and LCS
    uint8_t length;
    if (!_readByte(&length, timeout))           return PN532_TIMEOUT;
    uint8_t lcs;
    if (!_readByte(&lcs, timeout))              return PN532_TIMEOUT;
    if ((uint8_t)(length + lcs) != 0)           return PN532_INVALID_FRAME;

    if (length < 2) return PN532_INVALID_FRAME;

    // TFI (should be PN532TOHOST = 0xD5)
    uint8_t tfi;
    if (!_readByte(&tfi, timeout))              return PN532_TIMEOUT;
    if (tfi != PN532_PN532TOHOST)               return PN532_INVALID_FRAME;

    // Command code (one byte, discard — caller knows what it sent)
    uint8_t cmd;
    if (!_readByte(&cmd, timeout))              return PN532_TIMEOUT;

    // Remaining payload
    uint8_t payloadLen = length - 2;  // subtract TFI and cmd bytes
    if (payloadLen > len) payloadLen = len;

    uint8_t sum = tfi + cmd;
    for (int i = 0; i < payloadLen; i++) {
        if (!_readByte(&buf[i], timeout)) return PN532_TIMEOUT;
        sum += buf[i];
    }

    // DCS
    uint8_t dcs;
    if (!_readByte(&dcs, timeout)) return PN532_TIMEOUT;
    if ((uint8_t)(sum + dcs) != 0) return PN532_INVALID_FRAME;

    // Postamble
    _readByte(&b, timeout);   // 0x00 — ignore if missing

    return payloadLen;
}
