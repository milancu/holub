#include "web_server.h"
#include "pump.h"
#include "camera.h"
#include "index_html.h"
#include "config.h"

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "esp_camera.h"

// Boundary pro MJPEG multipart stream.
#define STREAM_BOUNDARY "frame123456789"

static AsyncWebServer g_server(80);

// ============================================================
// MJPEG stream state — alokuje se na heapu pro každého klienta,
// uvolní se v onDisconnect.
//
// Pozn.: ESPAsyncWebServer streamuje přes chunked callback, který
// se volá opakovaně, dokud nevrátí 0. My v něm jedeme stavový automat:
// získat frame -> poslat hlavičku části -> poslat JPEG body -> znovu.
// ============================================================
struct StreamCtx {
    enum Stage { GET_FRAME, SEND_HEADER, SEND_BODY } stage = GET_FRAME;
    camera_fb_t* fb = nullptr;
    String header;
    size_t headerPos = 0;
    size_t bodyPos   = 0;

    ~StreamCtx() { if (fb) esp_camera_fb_return(fb); }

    size_t fill(uint8_t* buf, size_t maxLen) {
        if (stage == GET_FRAME) {
            fb = esp_camera_fb_get();
            if (!fb) return 0;     // chyba kamery → ukončit stream
            header = "\r\n--" STREAM_BOUNDARY "\r\n"
                     "Content-Type: image/jpeg\r\n"
                     "Content-Length: " + String(fb->len) + "\r\n\r\n";
            headerPos = 0;
            bodyPos   = 0;
            stage     = SEND_HEADER;
        }
        if (stage == SEND_HEADER) {
            size_t remaining = header.length() - headerPos;
            size_t n = remaining < maxLen ? remaining : maxLen;
            memcpy(buf, header.c_str() + headerPos, n);
            headerPos += n;
            if (headerPos >= header.length()) stage = SEND_BODY;
            return n;
        }
        // SEND_BODY
        size_t remaining = fb->len - bodyPos;
        size_t n = remaining < maxLen ? remaining : maxLen;
        memcpy(buf, fb->buf + bodyPos, n);
        bodyPos += n;
        if (bodyPos >= fb->len) {
            esp_camera_fb_return(fb);
            fb    = nullptr;
            stage = GET_FRAME;       // další iterace si vezme nový frame
        }
        return n;
    }
};

// Pomocná funkce pro POST/GET ?ms=N
static uint32_t readMsParam(AsyncWebServerRequest* req) {
    if (req->hasParam("ms", true))  return req->getParam("ms", true)->value().toInt();
    if (req->hasParam("ms", false)) return req->getParam("ms", false)->value().toInt();
    return DEFAULT_SPRAY_MS;
}

namespace web {

void begin() {
    // ---------------- GET / ----------------
    g_server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html; charset=utf-8", INDEX_HTML);
    });

    // ---------------- GET /snapshot ----------------
    g_server.on("/snapshot", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!camera::isReady()) { req->send(503, "text/plain", "camera not available"); return; }
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) { req->send(500, "text/plain", "camera fb_get failed"); return; }

        size_t   len  = fb->len;
        uint8_t* copy = (uint8_t*)ps_malloc(len);
        if (!copy) copy = (uint8_t*)malloc(len);
        if (!copy) {
            esp_camera_fb_return(fb);
            req->send(500, "text/plain", "alloc failed");
            return;
        }
        memcpy(copy, fb->buf, len);
        esp_camera_fb_return(fb);

        AsyncWebServerResponse* resp = req->beginResponse(
            "image/jpeg", len,
            [copy, len](uint8_t* buf, size_t maxLen, size_t index) -> size_t {
                if (index >= len) return 0;
                size_t remaining = len - index;
                size_t n = remaining < maxLen ? remaining : maxLen;
                memcpy(buf, copy + index, n);
                return n;
            });
        req->onDisconnect([copy]() { free(copy); });
        req->send(resp);
    });

    // ---------------- GET /stream ----------------
    g_server.on("/stream", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!camera::isReady()) { req->send(503, "text/plain", "camera not available"); return; }
        auto* ctx = new StreamCtx();
        AsyncWebServerResponse* resp = req->beginChunkedResponse(
            "multipart/x-mixed-replace;boundary=" STREAM_BOUNDARY,
            [ctx](uint8_t* buf, size_t maxLen, size_t /*index*/) -> size_t {
                return ctx->fill(buf, maxLen);
            });
        req->onDisconnect([ctx]() { delete ctx; });
        req->send(resp);
    });

    // ---------------- POST /spray ----------------
    g_server.on("/spray", HTTP_POST, [](AsyncWebServerRequest* req) {
        uint32_t ms = readMsParam(req);

        // Tvrdý strop — i kdyby validace vrátila INVALID, držíme jistotu.
        if (ms > MAX_SPRAY_MS) ms = MAX_SPRAY_MS;

        switch (pump::spray(ms)) {
            case pump::SprayResult::OK:
                req->send(200, "application/json",
                          "{\"ok\":true,\"ms\":" + String(ms) + "}");
                break;
            case pump::SprayResult::IN_COOLDOWN:
                req->send(429, "application/json",
                          "{\"ok\":false,\"error\":\"cooldown\"}");
                break;
            case pump::SprayResult::ALREADY_ACTIVE:
                req->send(409, "application/json",
                          "{\"ok\":false,\"error\":\"already_active\"}");
                break;
            case pump::SprayResult::INVALID_DURATION:
                req->send(400, "application/json",
                          "{\"ok\":false,\"error\":\"invalid_ms\"}");
                break;
        }
    });

    // ---------------- GET /status ----------------
    g_server.on("/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        doc["uptime_ms"]     = millis();
        doc["free_heap"]     = ESP.getFreeHeap();
        doc["last_spray_ms"] = pump::lastSprayMs();
        doc["sprays_total"]  = pump::sprayCount();
        doc["pump_active"]   = pump::isActive();
        doc["camera_ok"]     = camera::isReady();
        doc["wifi_rssi"]     = WiFi.RSSI();

        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    g_server.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "text/plain", "404");
    });

    g_server.begin();
}

} // namespace web
