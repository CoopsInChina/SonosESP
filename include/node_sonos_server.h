#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Discovers and tracks the node-sonos-http-api server on the local network.
// Discovery probes every host on the device's /24 subnet for port 5005,
// verifies with a GET /zones request, then caches the URL in NVS.
class NodeSonosServer {
public:
    enum class State { IDLE, SCANNING, FOUND, NOT_FOUND };

    void begin();            // load cached URL from NVS; starts discovery if unreachable
    void startDiscovery();   // launch background subnet scan (safe to call anytime)
    void cancelDiscovery();
    void onCommFailure();    // call when HTTP to server fails — triggers re-discovery

    // Build and send a play request from a raw NFC URI (e.g. "spotify:album:2dfTV7C...")
    // room is the Sonos room name (spaces will be URL-encoded automatically)
    void sendPlayRequest(const String& nfcUri, const String& room);

    State       getState()    const { return _state; }
    bool        isFound()     const { return _state == State::FOUND; }
    String      getBaseUrl()  const { return _baseUrl; }   // e.g. "http://192.168.1.50:5005"

    // Called by the NFC screen to register a UI update callback
    using StatusCallback = void (*)();
    void setStatusCallback(StatusCallback cb) { _statusCb = cb; }

private:
    static void scanTask(void* param);
    bool        probeHost(const String& ip);
    bool        verifyHost(const String& ip);
    void        setFound(const String& ip);
    void        saveToNVS(const String& url);
    static String urlEncode(const String& s);
    static String buildPlayUrl(const String& base, const String& room, const String& nfcUri);

    String         _baseUrl;
    State          _state      = State::IDLE;
    TaskHandle_t   _taskHandle = nullptr;
    StatusCallback _statusCb   = nullptr;
};

extern NodeSonosServer sonosHttpServer;
