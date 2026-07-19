#include "reset.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <Esp.h>

void forceReset() {
    ESP.restart(); // well-documented, standard ESP32 reset -- no watchdog
                    // timing concerns here at all, unlike the AVR path below
    while (true) { } // never reached
}

#else
#include <avr/io.h>

void forceReset() {
    _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
    while (true) { } // never reached -- the write above resets the chip synchronously
}

#endif
