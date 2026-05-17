#pragma once
#include "esp_camera.h"

namespace camera {

// Inicializace OV2640 s AI-Thinker pinoutem. Vrací false při chybě.
bool begin();
bool isReady();

} // namespace camera
