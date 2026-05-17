#pragma once

// ============================================================
// Stub pro PAN/TILT serva.
// Při ENABLE_SERVOS == 0 jsou všechny funkce no-op (kompilují se ale).
// Po připojení hardwaru přepni v config.h na 1.
// ============================================================

namespace servos {

void begin();
void setPan(int angle);   // 0–180°
void setTilt(int angle);  // 0–180°

} // namespace servos
