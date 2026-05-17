#include "pump.h"
#include "config.h"

namespace pump {

// Low-level trigger relé: LOW = sepnuto (pumpa ON), HIGH = rozepnuto (pumpa OFF).
static inline void pumpOff() { digitalWrite(PUMP_PIN, HIGH); }
static inline void pumpOn()  { digitalWrite(PUMP_PIN, LOW);  }

static bool     g_active           = false;
static uint32_t g_sprayStartMs     = 0;
static uint32_t g_sprayDurationMs  = 0;
static uint32_t g_lastSprayStartMs = 0;   // pro /status
static uint32_t g_lastSprayEndMs   = 0;   // pro cooldown
static uint32_t g_sprayCount       = 0;

void begin() {
    // KRITICKÉ: tento pinMode + digitalWrite musí proběhnout DŘÍVE než
    // cokoli jiného v setup(), aby pumpa nezůstala viset v ON po restartu
    // (relé v low-level triggeru sepne při floating IN).
    pinMode(PUMP_PIN, OUTPUT);
    pumpOff();
}

SprayResult spray(uint32_t durationMs) {
    if (durationMs == 0 || durationMs > MAX_SPRAY_MS) {
        return SprayResult::INVALID_DURATION;
    }
    if (g_active) {
        return SprayResult::ALREADY_ACTIVE;
    }
    // První spray po bootu má g_lastSprayEndMs == 0 → cooldown se přeskočí.
    if (g_lastSprayEndMs != 0 && (millis() - g_lastSprayEndMs) < COOLDOWN_MS) {
        return SprayResult::IN_COOLDOWN;
    }

    g_active           = true;
    g_sprayStartMs     = millis();
    g_sprayDurationMs  = durationMs;
    g_lastSprayStartMs = g_sprayStartMs;
    g_sprayCount++;
    pumpOn();
    return SprayResult::OK;
}

void update() {
    if (!g_active) return;
    if ((millis() - g_sprayStartMs) >= g_sprayDurationMs) {
        pumpOff();
        g_active         = false;
        g_lastSprayEndMs = millis();
    }
}

bool     isActive()    { return g_active; }
uint32_t lastSprayMs() { return g_lastSprayStartMs; }
uint32_t sprayCount()  { return g_sprayCount; }

} // namespace pump
