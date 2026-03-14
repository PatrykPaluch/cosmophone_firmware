#include "rainbow.h"
#include "display.h"
#include "touch.h"
#include <Arduino.h>
#include <string.h>

namespace apps {
namespace rainbow {

static const int W = 480;
static const int H = 480;

static uint16_t hsv565(int h) {
    int region = h / 60;
    int t      = (h % 60) * 256 / 60;
    uint8_t r, g, b;
    switch (region) {
        case 0:  r = 255;     g = t;       b = 0;       break;
        case 1:  r = 255 - t; g = 255;     b = 0;       break;
        case 2:  r = 0;       g = 255;     b = t;       break;
        case 3:  r = 0;       g = 255 - t; b = 255;     break;
        case 4:  r = t;       g = 0;       b = 255;     break;
        default: r = 255;     g = 0;       b = 255 - t; break;
    }
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
}

static bool justTouched(bool &prev) {
    int tx, ty;
    bool cur = sys::touch::getTouch(tx, ty);
    bool rising = cur && !prev;
    prev = cur;
    return rising;
}

void run() {
    static uint16_t extPal[960];
    for (int i = 0; i < 480; i++) {
        extPal[i] = extPal[i + 480] = hsv565(i * 360 / 480);
    }

    uint16_t *fb   = sys::display::getFramebuffer();
    int       offs = 0;
    bool      prev = false;

    while (!justTouched(prev)) {
        uint16_t *row = &extPal[offs];
        for (int y = 0; y < H; y++) {
            memcpy(fb + y * W, row, W * 2);
        }
        sys::display::flushFramebuffer();
        if (++offs >= 480) offs = 0;
    }
}

}  // namespace rainbow
}  // namespace apps
