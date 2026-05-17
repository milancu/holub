# Holub — automatický postřikovač holubů

## Co to je
Monorepo: ESP32-CAM s vodní pumpou (postřik) + Raspberry Pi 5 s detekcí ptáků (YOLOv8). RPi stahuje snímky z ESP32, detekuje holuby a automaticky spouští pumpu.

## Hardware
- ESP32-CAM AI-Thinker s externí U.FL anténou
- Napájení 5V přes LM2596 z 12V adaptéru
- 1-kanálové 5V relé (low-level trigger): GPIO13 — LOW = pumpa ON, HIGH = pumpa OFF
- DC vodní pumpa R385 12V spínaná přes relé
- Serva PAN (GPIO14) / TILT (GPIO15) — kód připravený, ale deaktivovaný (`ENABLE_SERVOS 0` v config.h)

## Struktura monorepa
```
esp32/                    # PlatformIO projekt — firmware ESP32-CAM
  platformio.ini          # espressif32 @ 6.7.0, board esp32cam, huge_app partition
  include/
    config.h              # WiFi SSID/PASS, piny, limity — GITIGNORED
    config.example.h      # šablona pro config.h
  src/
    main.cpp              # setup/loop, WiFi, mDNS, watchdog
    pump.cpp/.h           # neblokující spray s hard capem 3s a cooldownem 2s
    camera.cpp/.h         # OV2640 init (AI-Thinker pinout)
    servos.cpp/.h         # PAN/TILT stub (#if ENABLE_SERVOS)
    web_server.cpp/.h     # ESPAsyncWebServer — routy níže
    index_html.h          # PROGMEM HTML stránka
rpi/                      # Python + Docker — detekce holubů na RPi 5
  detector.py             # hlavní loop: snapshot → YOLOv8 → spray
  requirements.txt
  Dockerfile              # deploy přes Coolify
```

## HTTP endpointy
- `GET /` — HTML stránka s live MJPEG preview a tlačítkem Spray
- `GET /stream` — MJPEG multipart stream
- `GET /snapshot` — jeden JPEG
- `POST /spray?ms=500` — spustí pumpu (50–3000ms), vrací 429 při cooldownu
- `GET /status` — JSON: uptime_ms, free_heap, last_spray_ms, sprays_total, pump_active, wifi_rssi

## Bezpečnostní invarianty
- `pump::begin()` (pin HIGH = OFF) je PRVNÍ volání v setup(), před Serial/kamerou/WiFi
- Hard cap 3000ms na spray, cooldown 2000ms mezi spraye
- Watchdog 10s (esp_task_wdt) — panic & restart při zacyklení
- Brownout detector ponechán aktivní — restart = pumpa OFF díky setup() pořadí

## Knihovny
- mathieucarbou/ESPAsyncWebServer (ne me-no-dev — ten má race conditiony)
- ArduinoJson 7.x
- ESP32Servo (připraveno pro budoucí serva)

## Flash procedura (ESP32)
- Programátor: LaskaKit CH340 USB-C pro ESP32-CAM (LA161081)
- Zasuneš ESP32-CAM do programátoru, připojíš USB-C, `cd esp32 && pio run -t upload` — hotovo
- Programátor řeší GPIO0 boot mode i reset automaticky, žádné jumperování
- Upload port: COM4

## Deploy RPi (Coolify)
- Coolify nasadí Docker kontejner z `rpi/` složky
- Kontejner stahuje snapshoty z ESP32 a spouští YOLO detekci
- Při detekci ptáka automaticky volá POST /spray

## Konvence
- Komentáře česky, identifikátory anglicky
