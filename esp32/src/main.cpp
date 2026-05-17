#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "esp_task_wdt.h"
#include "esp_system.h"

#include "config.h"
#include "pump.h"
#include "camera.h"
#include "servos.h"
#include "web_server.h"

void setup() {
    // ============================================================
    // KRITICKÉ: úplně první akce po bootu — pin pumpy na HIGH (OFF).
    // Před Serial, kamerou, WiFi. Pokud by se ESP32 restartoval během
    // aktivního spraye, pumpa zhasne dřív, než stihne cokoli dalšího.
    // ============================================================
    pump::begin();

    Serial.begin(115200);
    delay(100);
    Serial.println();
    Serial.println("[boot] start");

    // Diagnostika důvodu posledního restartu (panic / brownout / WDT)
    Serial.printf("[boot] reset reason: %d\n", (int)esp_reset_reason());

    // Watchdog — restartne ESP, pokud loop() neudělá esp_task_wdt_reset() včas.
    esp_task_wdt_init(WDT_TIMEOUT_S, true);  // true = panic & restart
    esp_task_wdt_add(NULL);                  // přidat loop task

    // Kamera (musí být před prvním HTTP requestem)
    if (!camera::begin()) {
        Serial.println("[boot] camera init FAILED — pokracuji bez kamery");
    }

    // Serva (no-op pokud ENABLE_SERVOS == 0)
    servos::begin();

    // WiFi (STA, DHCP)
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);                    // lepší latence pro stream
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("[wifi] connecting to %s", WIFI_SSID);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 20000) {
        delay(250);
        Serial.print(".");
        esp_task_wdt_reset();
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n[wifi] FAILED — restart");
        ESP.restart();
    }
    Serial.printf("\n[wifi] IP: %s  RSSI: %d\n",
                  WiFi.localIP().toString().c_str(), (int)WiFi.RSSI());

    // mDNS (přístup přes http://holub.local)
    if (MDNS.begin(MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("[mdns] http://%s.local\n", MDNS_HOSTNAME);
    } else {
        Serial.println("[mdns] start FAILED");
    }

    // HTTP server
    web::begin();
    Serial.println("[boot] ready");
}

void loop() {
    pump::update();          // neblokující stavový automat pumpy
    esp_task_wdt_reset();    // krmení watchdogu
    delay(1);                // yield RTOS scheduleru (WiFi/AsyncTCP task)
}
