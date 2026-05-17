#pragma once
#include <Arduino.h>

// ============================================================
// Bezpečné ovládání pumpy přes relé.
//
// Návrh: neblokující. spray() jen označí pumpu jako aktivní a uloží start.
// Vlastní vypnutí proběhne v update() volaném z loop(), takže HTTP server
// zůstane responzivní i během spraye.
// ============================================================

namespace pump {

enum class SprayResult {
    OK,
    IN_COOLDOWN,
    ALREADY_ACTIVE,
    INVALID_DURATION,
};

// MUSÍ být první volání v setup() — okamžitě zhasne pumpu.
void begin();

// Neblokující: nastaví pumpu ON, vrátí výsledek.
// Vrací IN_COOLDOWN pokud od posledního spraye uplynulo méně než COOLDOWN_MS.
// Vrací INVALID_DURATION pokud ms == 0 nebo > MAX_SPRAY_MS.
SprayResult spray(uint32_t durationMs);

// Volej z loop() — vypne pumpu po doběhnutí.
void update();

bool     isActive();
uint32_t lastSprayMs();   // millis() okamžiku posledního startu (0 = nikdy)
uint32_t sprayCount();

} // namespace pump
