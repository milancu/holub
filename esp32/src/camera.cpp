#include "camera.h"
#include <Arduino.h>

// ============================================================
// AI-Thinker ESP32-CAM pinout — fixní, nelze měnit.
// ============================================================
#define PWDN_GPIO_NUM   32
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM   26
#define SIOC_GPIO_NUM   27
#define Y9_GPIO_NUM     35
#define Y8_GPIO_NUM     34
#define Y7_GPIO_NUM     39
#define Y6_GPIO_NUM     36
#define Y5_GPIO_NUM     21
#define Y4_GPIO_NUM     19
#define Y3_GPIO_NUM     18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM  25
#define HREF_GPIO_NUM   23
#define PCLK_GPIO_NUM   22

namespace camera {

static bool g_ready = false;

bool isReady() { return g_ready; }

bool begin() {
    Serial.printf("[camera] PSRAM: %s (%d bytes)\n",
                  psramFound() ? "ANO" : "NE", ESP.getFreePsram());

    // PWDN power-cycle — probudí kameru z power-down stavu
    pinMode(PWDN_GPIO_NUM, OUTPUT);
    digitalWrite(PWDN_GPIO_NUM, HIGH);  // power down
    delay(200);
    digitalWrite(PWDN_GPIO_NUM, LOW);   // power up
    delay(500);                         // OV2640 potřebuje čas na stabilizaci

    camera_config_t cfg = {};
    cfg.ledc_channel  = LEDC_CHANNEL_0;
    cfg.ledc_timer    = LEDC_TIMER_0;
    cfg.pin_d0        = Y2_GPIO_NUM;
    cfg.pin_d1        = Y3_GPIO_NUM;
    cfg.pin_d2        = Y4_GPIO_NUM;
    cfg.pin_d3        = Y5_GPIO_NUM;
    cfg.pin_d4        = Y6_GPIO_NUM;
    cfg.pin_d5        = Y7_GPIO_NUM;
    cfg.pin_d6        = Y8_GPIO_NUM;
    cfg.pin_d7        = Y9_GPIO_NUM;
    cfg.pin_xclk      = XCLK_GPIO_NUM;
    cfg.pin_pclk      = PCLK_GPIO_NUM;
    cfg.pin_vsync     = VSYNC_GPIO_NUM;
    cfg.pin_href      = HREF_GPIO_NUM;
    cfg.pin_sccb_sda  = SIOD_GPIO_NUM;
    cfg.pin_sccb_scl  = SIOC_GPIO_NUM;
    cfg.pin_pwdn      = PWDN_GPIO_NUM;
    cfg.pin_reset     = RESET_GPIO_NUM;
    cfg.xclk_freq_hz  = 20000000;
    cfg.pixel_format  = PIXFORMAT_JPEG;

    // S PSRAM si můžeme dovolit větší rozlišení a 2 frame buffery
    // (plynulejší MJPEG stream — kamera plní jeden buffer, my odesíláme druhý).
    if (psramFound()) {
        cfg.frame_size   = FRAMESIZE_VGA;        // 640x480
        cfg.jpeg_quality = 12;                   // 0–63, nižší = lepší kvalita
        cfg.fb_count     = 2;
        cfg.fb_location  = CAMERA_FB_IN_PSRAM;
        cfg.grab_mode    = CAMERA_GRAB_LATEST;
    } else {
        cfg.frame_size   = FRAMESIZE_QVGA;       // 320x240
        cfg.jpeg_quality = 15;
        cfg.fb_count     = 1;
        cfg.fb_location  = CAMERA_FB_IN_DRAM;
        cfg.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
    }

    // Některé OV2640 moduly potřebují víc pokusů
    esp_err_t err = ESP_FAIL;
    for (int attempt = 1; attempt <= 3; attempt++) {
        err = esp_camera_init(&cfg);
        if (err == ESP_OK) break;
        Serial.printf("[camera] pokus %d/3 selhal: 0x%x\n", attempt, err);
        esp_camera_deinit();
        // PWDN power-cycle mezi pokusy
        digitalWrite(PWDN_GPIO_NUM, HIGH);
        delay(200);
        digitalWrite(PWDN_GPIO_NUM, LOW);
        delay(500);
    }
    if (err != ESP_OK) {
        Serial.printf("[camera] init FAILED po 3 pokusech\n");
        return false;
    }

    // Lehké ladění pro OV2640
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_brightness(s, 0);
        s->set_saturation(s, 0);
    }
    g_ready = true;
    return true;
}

} // namespace camera
