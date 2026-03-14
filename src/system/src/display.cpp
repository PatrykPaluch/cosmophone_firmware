#include "display.h"
#include <string.h>   // memcmp, memcpy
#include "esp_heap_caps.h"

#define GFX_BL 38
static const int FB_W = 480;
static const int FB_H = 480;
static const int ROW_BYTES = FB_W * 2;   // 960 bytes per row

static Arduino_ESP32RGBPanel *_bus = new Arduino_ESP32RGBPanel(
    39, 48, 47, 18, 17, 16, 21,
    11, 12, 13, 14, 0,
    8, 20, 3, 46, 9, 10,
    4, 5, 6, 7, 15);

static Arduino_ST7701_RGBPanel *_panel = new Arduino_ST7701_RGBPanel(
    _bus, GFX_NOT_DEFINED, 0, true, FB_W, FB_H,
    st7701_type1_init_operations, sizeof(st7701_type1_init_operations), true,
    10, 8, 50, 10, 8, 20);

// Global gfx for drawing (used by apps and menu).
Arduino_GFX *gfx = _panel;

static uint16_t *_displayFb  = nullptr;
static uint16_t *_backbuffer = nullptr;

namespace sys {
namespace display {

void init() {
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
    gfx->begin(16000000);
    _displayFb = _panel->getFramebuffer();
    gfx->fillScreen(BLACK);
}

uint16_t *getFramebuffer() {
    return _displayFb;
}

void flushFramebuffer() {
    Cache_WriteBack_Addr((uint32_t)_displayFb, FB_W * FB_H * 2);
}

static void ensureBackbuffer() {
    if (!_backbuffer) {
        _backbuffer = (uint16_t *)heap_caps_malloc(FB_W * FB_H * 2, MALLOC_CAP_SPIRAM);
    }
}

void beginFrame() {
    ensureBackbuffer();
    _panel->setFramebuffer(_backbuffer);
}

void endFrame() {
    uint16_t *back  = _backbuffer;
    uint16_t *front = _displayFb;

    int batchStart = -1;

    for (int y = 0; y <= FB_H; y++) {
        bool dirty = (y < FB_H) && (memcmp(
            back  + (int32_t)y * FB_W,
            front + (int32_t)y * FB_W,
            ROW_BYTES) != 0);

        if (dirty) {
            memcpy(front + (int32_t)y * FB_W,
                   back  + (int32_t)y * FB_W,
                   ROW_BYTES);
            if (batchStart < 0) batchStart = y;
        } else if (batchStart >= 0) {
            Cache_WriteBack_Addr(
                (uint32_t)(front + (int32_t)batchStart * FB_W),
                (uint32_t)(y - batchStart) * ROW_BYTES);
            batchStart = -1;
        }
    }

    _panel->setFramebuffer(_displayFb);
}

}  // namespace display
}  // namespace sys
