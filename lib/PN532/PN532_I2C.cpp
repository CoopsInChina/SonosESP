#include "PN532_I2C.h"

const uint8_t PN532_I2C::ACK_FRAME[6] = { 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00 };

// ---------------------------------------------------------------------------
// Wait until PN532 sets its ready byte (first I2C read byte) to 0x01
// ---------------------------------------------------------------------------
bool PN532_I2C::_waitReady(uint16_t timeout) {
    uint32_t t0 = millis();
    while (millis() - t0 < timeout) {
        _wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, (uint8_t)1);
        if (_wire.available()) {
            uint8_t status = _wire.read();
            if (status == 0x01) return true;
        }
        delay(5);
    }
    return false;
}

// ---------------------------------------------------------------------------
// writeCommand — send a PN532 frame and verify ACK
// ---------------------------------------------------------------------------
int8_t PN532_I2C::writeCommand(const uint8_t* header, uint8_t hlen,
                                const uint8_t* body,   uint8_t blen) {
    // Build checksum over TFI + header + body
    uint8_t len = hlen + blen + 1;   // +1 for TFI byte
    uint8_t sum = PN532_HOSTTOPN532;
    for (int i = 0; i < hlen; i++) sum += header[i];
    for (int i = 0; i < blen; i++) sum += (body ? body[i] : 0);

    _wire.beginTransmission(PN532_I2C_ADDRESS);
    _wire.write(PN532_PREAMBLE);
    _wire.write(PN532_STARTCODE1);
    _wire.write(PN532_STARTCODE2);
    _wire.write(len);
    _wire.write(~len + 1);              // LCS
    _wire.write(PN532_HOSTTOPN532);
    for (int i = 0; i < hlen; i++) _wire.write(header[i]);
    if (body && blen) {
        for (int i = 0; i < blen; i++) _wire.write(body[i]);
    }
    _wire.write(~sum + 1);              // DCS
    _wire.write(PN532_POSTAMBLE);
    _wire.endTransmission();

    // Wait for PN532 ready, then read back the ACK frame
    if (!_waitReady(100)) {
        return PN532_TIMEOUT;
    }

    // Read status byte (1) + ACK frame (6) = 7 bytes
    _wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, (uint8_t)7);
    if (_wire.available() < 7) return PN532_INVALID_ACK;

    _wire.read();  // status byte (already confirmed 0x01 in _waitReady)
    uint8_t ack[6];
    for (int i = 0; i < 6; i++) ack[i] = _wire.read();

    if (memcmp(ack, ACK_FRAME, 6) != 0) return PN532_INVALID_ACK;
    return 0;
}

// ---------------------------------------------------------------------------
// readResponse — wait for PN532 ready, then read response frame
// ---------------------------------------------------------------------------
int16_t PN532_I2C::readResponse(uint8_t buf[], uint8_t len, uint16_t timeout) {
    if (!_waitReady(timeout)) return PN532_TIMEOUT;

    // Read up to len+9 bytes: status(1) + preamble(3) + len(2) + TFI(1) + cmd(1) + payload(n) + DCS(1) + postamble(1)
    uint8_t raw[len + 10];
    uint8_t toRead = len + 10;
    _wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, toRead);

    uint8_t idx = 0;
    while (_wire.available() && idx < toRead) {
        raw[idx++] = _wire.read();
    }

    // raw[0] = status byte (0x01), raw[1..3] = 0x00 0x00 0xFF (preamble+start)
    if (idx < 7) return PN532_INVALID_FRAME;
    if (raw[1] != 0x00 || raw[2] != 0x00 || raw[3] != 0xFF) return PN532_INVALID_FRAME;

    uint8_t length = raw[4];
    uint8_t lcs    = raw[5];
    if ((uint8_t)(length + lcs) != 0) return PN532_INVALID_FRAME;
    if (length < 2) return PN532_INVALID_FRAME;

    uint8_t tfi = raw[6];
    uint8_t cmd = raw[7];
    if (tfi != PN532_PN532TOHOST) return PN532_INVALID_FRAME;

    uint8_t payloadLen = length - 2;
    if (payloadLen > len) payloadLen = len;

    uint8_t sum = tfi + cmd;
    for (int i = 0; i < payloadLen; i++) {
        buf[i] = raw[8 + i];
        sum += buf[i];
    }

    uint8_t dcs = raw[8 + payloadLen];
    if ((uint8_t)(sum + dcs) != 0) return PN532_INVALID_FRAME;

    return payloadLen;
}
