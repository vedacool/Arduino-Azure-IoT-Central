#include "reset.h"
#include <avr/io.h>

void forceReset() {
    _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
    while (true) { } // never reached -- the write above resets the chip synchronously
}
