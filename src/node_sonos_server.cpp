#include "node_sonos_server.h"
#include "config.h"
#include "ui_common.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>

NodeSonosServer sonosHttpServer;

extern Preferences wifiPrefs;

// ── Public API ────────────────────────────────────────────────────────────────

void NodeSonosServer::begin() {
    wifiPrefs.begin(NVS_NAMESPACE, true);
    String cached = wifiPrefs.getString(NVS_KEY_SONOS_HTTP_SERVER, "");
    wifiPrefs.end();

    if (cached.length() == 0) {
        Serial.println("[SonosHTTP] No cached server — starting discovery");
        startDiscovery();
        return;
    }

    // Extract IP from cached URL "http://x.x.x.x:5005"
    String ip = cached;
    ip.replace("http://", "");
    int colon = ip.indexOf(':');
    if (colon > 0) ip = ip.substring(0, colon);

    if (verifyHost(ip)) {
        _baseUrl = cached;
        _state   = State::FOUND;
        Serial.printf("[SonosHTTP] Cached server reachable: %s\n", _baseUrl.c_str());
        if (_statusCb) _statusCb();
    } else {
        Serial.printf("[SonosHTTP] Cached server %s unreachable — starting discovery\n", cached.c_str());
        startDiscovery();
    }
}

void NodeSonosServer::startDiscovery() {
    if (_state == State::SCANNING) return;
    cancelDiscovery();
    _state = State::SCANNING;
    if (_statusCb) _statusCb();
    Serial.println("[SonosHTTP] Starting subnet scan...");
    xTaskCreatePinnedToCore(scanTask, "SonosHTTPScan", 4096, this, 1, &_taskHandle, 0);
}

void NodeSonosServer::cancelDiscovery() {
    if (_taskHandle) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
}

void NodeSonosServer::onCommFailure() {
    if (_state == State::SCANNING) return;
    Serial.println("[SonosHTTP] Communication failure — re-running discovery");
    _state   = State::NOT_FOUND;
    _baseUrl = "";
    if (_statusCb) _statusCb();
    startDiscovery();
}

// ── Background scan task ──────────────────────────────────────────────────────

void NodeSonosServer::scanTask(void* param) {
    NodeSonosServer* self = static_cast<NodeSonosServer*>(param);

    IPAddress localIp = WiFi.localIP();
    if (localIp[0] == 0) {
        Serial.println("[SonosHTTP] No IP — scan aborted");
        self->_state = State::NOT_FOUND;
        if (self->_statusCb) self->_statusCb();
        self->_taskHandle = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    uint8_t ownOctet = localIp[3];

    // Build probe order: gateway neighbour first (.1–.10), then mid-range
    // (.100–.200, where DHCP computers typically land), then full sweep.
    // This finds a server at a typical desktop/server IP in < 5 seconds.
    String prefix = String(localIp[0]) + "." + String(localIp[1]) + "." + String(localIp[2]) + ".";

    Serial.printf("[SonosHTTP] Scanning %sx for port %d\n", prefix.c_str(), SONOS_HTTP_PORT);

    for (int i = 1; i <= 254; i++) {
        if (i == ownOctet) continue;
        if (i % 10 == 0) Serial.printf("[SonosHTTP] Probing %s%d\n", prefix.c_str(), i);
        String ip = prefix + String(i);
        if (self->probeHost(ip) && self->verifyHost(ip)) {
            self->setFound(ip);
            self->_taskHandle = nullptr;
            vTaskDelete(nullptr);
            return;
        }
        if ((i % 16) == 0) vTaskDelay(pdMS_TO_TICKS(5));
    }

    Serial.println("[SonosHTTP] Scan complete — server not found");
    self->_state = State::NOT_FOUND;
    if (self->_statusCb) self->_statusCb();
    self->_taskHandle = nullptr;
    vTaskDelete(nullptr);
}

// ── Probe helpers ─────────────────────────────────────────────────────────────

bool NodeSonosServer::probeHost(const String& ip) {
    // WiFiClient.setTimeout() only controls read timeout, not connect timeout.
    // lwIP's ARP/SYN retry takes ~2.5s for non-existent hosts regardless.
    // Use a raw non-blocking socket + select() for a true short timeout.
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;

    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(SONOS_HTTP_PORT);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);
    struct timeval tv{ 0, SONOS_HTTP_PROBE_TIMEOUT_MS * 1000 };

    bool ok = (select(sock + 1, nullptr, &fdset, nullptr, &tv) > 0);
    close(sock);
    return ok;
}

bool NodeSonosServer::verifyHost(const String& ip) {
    if (!xSemaphoreTake(network_mutex, pdMS_TO_TICKS(2000))) return false;

    WiFiClient client;
    HTTPClient http;
    String url = "http://" + ip + ":" + String(SONOS_HTTP_PORT) + "/zones";
    http.begin(client, url);
    http.setTimeout(1000);
    int code = http.GET();
    String body = (code == 200) ? http.getString() : "";
    http.end();

    xSemaphoreGive(network_mutex);

    // node-sonos-http-api /zones returns a JSON array
    return (code == 200 && body.startsWith("["));
}

void NodeSonosServer::setFound(const String& ip) {
    _baseUrl = "http://" + ip + ":" + String(SONOS_HTTP_PORT);
    _state   = State::FOUND;
    saveToNVS(_baseUrl);
    Serial.printf("[SonosHTTP] Server found: %s\n", _baseUrl.c_str());
    if (_statusCb) _statusCb();
}

void NodeSonosServer::saveToNVS(const String& url) {
    wifiPrefs.begin(NVS_NAMESPACE, false);
    wifiPrefs.putString(NVS_KEY_SONOS_HTTP_SERVER, url);
    wifiPrefs.end();
}

// ── Play request ──────────────────────────────────────────────────────────────

String NodeSonosServer::urlEncode(const String& s) {
    String out;
    out.reserve(s.length() * 3);
    for (char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == ':') {
            out += c;
        } else if (c == ' ') {
            out += "%20";
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", (uint8_t)c);
            out += buf;
        }
    }
    return out;
}

String NodeSonosServer::buildPlayUrl(const String& base, const String& room, const String& nfcUri) {
    // nfcUri format: "service:type:ID"
    // e.g. "spotify:album:2dfTV7CktUEBkZCHiB7VQB"
    //      "spotify:playlist:32O0SSXDNWDrMievPkV0Im"
    //      "apple:album:1234567890"
    //      "apple:playlist:pl.abcdef"

    int first  = nfcUri.indexOf(':');
    int second = nfcUri.indexOf(':', first + 1);
    if (first < 0 || second < 0) {
        Serial.printf("[SonosHTTP] Unrecognised URI format: %s\n", nfcUri.c_str());
        return "";
    }

    String service = nfcUri.substring(0, first);
    String type    = nfcUri.substring(first + 1, second);
    String id      = nfcUri.substring(second + 1);
    String encodedRoom = urlEncode(room);

    service.toLowerCase();
    type.toLowerCase();

    if (service == "spotify") {
        if (type == "album") {
            // http://<server>/<room>/spotify/now/spotify:album:<id>
            return base + "/" + encodedRoom + "/spotify/now/spotify:album:" + id;
        } else if (type == "playlist") {
            // http://<server>/<room>/spotify/now/spotify:user:spotify:playlist:<id>
            return base + "/" + encodedRoom + "/spotify/now/spotify:user:spotify:playlist:" + id;
        }
    } else if (service == "apple" || service == "applemusic") {
        if (type == "album") {
            // http://<server>/<room>/applemusic/now/album:<id>
            return base + "/" + encodedRoom + "/applemusic/now/album:" + id;
        } else if (type == "playlist") {
            // http://<server>/<room>/applemusic/now/playlist:<id>
            return base + "/" + encodedRoom + "/applemusic/now/playlist:" + id;
        }
    }

    Serial.printf("[SonosHTTP] Unsupported service/type: %s/%s\n", service.c_str(), type.c_str());
    return "";
}

void NodeSonosServer::sendPlayRequest(const String& nfcUri, const String& room) {
    if (!isFound()) {
        Serial.println("[SonosHTTP] sendPlayRequest: server not found — ignoring");
        return;
    }
    if (room.length() == 0) {
        Serial.println("[SonosHTTP] sendPlayRequest: no room selected — ignoring");
        return;
    }

    String url = buildPlayUrl(_baseUrl, room, nfcUri);
    if (url.length() == 0) return;

    Serial.printf("[SonosHTTP] → %s\n", url.c_str());

    if (!xSemaphoreTake(network_mutex, pdMS_TO_TICKS(3000))) {
        Serial.println("[SonosHTTP] sendPlayRequest: network_mutex timeout");
        return;
    }

    // Clear queue before play so NFC taps don't accumulate tracks indefinitely.
    // Both requests are to the same local server so clearqueue completes in <200ms —
    // imperceptible against the 2-4s Sonos needs to start playing.
    {
        String clearUrl = _baseUrl + "/" + urlEncode(room) + "/clearqueue";
        WiFiClient clearClient;
        HTTPClient clearHttp;
        clearHttp.begin(clearClient, clearUrl);
        clearHttp.setTimeout(1000);
        int clearCode = clearHttp.GET();
        Serial.printf("[SonosHTTP] clearqueue → %d\n", clearCode);
        clearHttp.end();
    }

    WiFiClient client;
    HTTPClient http;
    http.begin(client, url);
    // node-sonos-http-api queues the command and responds within 1-2s.
    // Keeping the mutex for 8s blocks the SOAP polling task and art download task.
    // A -11 timeout just means we didn't receive the ack — Sonos still plays.
    http.setTimeout(2000);
    int code = http.GET();
    if (code > 0) {
        String body = http.getString();
        Serial.printf("[SonosHTTP] Response %d: %s\n", code, body.c_str());
    } else {
        http.end();
        xSemaphoreGive(network_mutex);
        // Only re-discover on connection-level failures — a timeout (-11) means
        // the request was sent and Sonos is executing it; don't treat as server lost
        if (code == -1 || code == -4) {
            Serial.printf("[SonosHTTP] Connection failure (%d) — triggering re-discovery\n", code);
            onCommFailure();
        } else {
            Serial.printf("[SonosHTTP] Request warning (%d) — server likely still OK\n", code);
        }
        return;
    }
    http.end();
    xSemaphoreGive(network_mutex);
}
