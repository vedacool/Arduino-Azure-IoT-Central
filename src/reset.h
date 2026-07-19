#ifndef RESET_H
#define RESET_H

// Triggers an immediate, full device reset. Two implementations, in
// reset.cpp:
//
// ESP32: ESP.restart(), the standard, well-documented Arduino-ESP32 reset --
// no watchdog timing concerns here at all; this one is straightforward.
//
// megaAVR (Uno WiFi Rev2 / Nano Every): the dedicated software-reset
// register (RSTCTRL.SWRR) rather than the watchdog timer. Confirmed against
// two independent sources: the official megaAVR-0 datasheet ("When SWRE is
// written to 1, a software reset will occur") and Arduino's own bootloader
// source for this exact chip (arduino/ArduinoCore-megaavr, bootloaders/
// boot.c), which uses this exact line. Chosen specifically because it's an
// immediate, deterministic trigger -- not a timed countdown, so it doesn't
// have the watchdog's documented timing-accuracy quirks on this chip family.
// NOT compile-tested in this environment: <avr/io.h>'s RSTCTRL struct is
// part of the device-specific header that ships only through Arduino's own
// Board Manager toolchain, which this project's sandbox couldn't reach. The
// two sources above independently agree on the exact line used here, but
// verify this specific function resets the board as expected on real
// hardware before depending on it for unattended operation.
void forceReset();

#endif
