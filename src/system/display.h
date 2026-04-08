#pragma once
#include <display/Arduino_RGB_Display.h>
#include <sys/types.h>

namespace sys {
namespace display {

void init();

// Standard back button (top-left). Draw it, then use backButtonTapped(tx, ty) in your touch loop.
void drawBackButton(uint16_t bg = 0x2145, uint16_t border = 0x07FF, uint16_t textColor = 0xFFFF);
bool backButtonTapped(int tx, int ty);

// Returns the visible display framebuffer for direct pixel writes.
// After writing, call flushDirect() to make the DMA see updated data.
uint16_t *getDirectFramebuffer();
uint16_t *getBufferedFramebuffer();

void flushDirect();
void flush();

}  // namespace display
}  // namespace sys

// Convenience: global gfx pointer for drawing.
extern Arduino_GFX *gfx;