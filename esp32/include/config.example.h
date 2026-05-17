#pragma once

// ============================================================
// Šablona konfigurace.
//
// 1) Zkopíruj tento soubor na include/config.h
// 2) Vyplň WIFI_SSID a WIFI_PASSWORD
// 3) config.h je v .gitignore, takže se nedostane do gitu.
// ============================================================

// --- WiFi ---
#define WIFI_SSID        "TVOJE_SIT"
#define WIFI_PASSWORD    "TVOJE_HESLO"

// --- mDNS hostname (přístup přes http://holub.local) ---
#define MDNS_HOSTNAME    "holub"

// --- GPIO piny ---
// POZOR: relé na GPIO13 — pin při bootu chvilku floatuje, než ho kód
// nastaví na HIGH. Doporučeno přidat externí 10k pull-up na linku IN
// proti 5V, jinak může relé na začátku krátce sepnout.
#define PUMP_PIN         13      // relé pumpy (low-level trigger: LOW = ON, HIGH = OFF)
#define SERVO_PAN_PIN    14      // servo PAN  (zatím nepřipojeno)
#define SERVO_TILT_PIN   15      // servo TILT (zatím nepřipojeno)

// --- Aktivace serv ---
// 0 = kód se kompiluje, ale serva jsou neaktivní (žádný attach).
// Až přijdou serva, přepni na 1.
#define ENABLE_SERVOS    0

// --- Bezpečnostní limity pumpy ---
#define MAX_SPRAY_MS     3000    // tvrdý strop trvání spraye
#define COOLDOWN_MS      2000    // minimální pauza mezi spraye
#define DEFAULT_SPRAY_MS 500     // default pokud klient nepošle ms

// --- Watchdog ---
#define WDT_TIMEOUT_S    10      // restart, pokud loop() nedoběhne za N sekund
