#include "servos.h"
#include "config.h"
#include <Arduino.h>

#if ENABLE_SERVOS
  #include <ESP32Servo.h>
  static Servo g_pan;
  static Servo g_tilt;
#endif

namespace servos {

void begin() {
#if ENABLE_SERVOS
    g_pan.attach(SERVO_PAN_PIN);
    g_tilt.attach(SERVO_TILT_PIN);
    g_pan.write(90);
    g_tilt.write(90);
#endif
}

void setPan(int angle) {
#if ENABLE_SERVOS
    angle = constrain(angle, 0, 180);
    g_pan.write(angle);
#else
    (void)angle;
#endif
}

void setTilt(int angle) {
#if ENABLE_SERVOS
    angle = constrain(angle, 0, 180);
    g_tilt.write(angle);
#else
    (void)angle;
#endif
}

} // namespace servos
