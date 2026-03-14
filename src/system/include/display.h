#pragma once
#include <Arduino_GFX_Library.h>

namespace sys {
namespace display {

void init();

// ── Direct framebuffer access (used by full-screen demos) ────────────────────
// Layout: fb[y * 480 + x] = RGB565 pixel.
// After a bulk write call flushFramebuffer() to make the DMA engine
// see the updated data (flushes CPU D-cache → PSRAM).
uint16_t *getFramebuffer();
void      flushFramebuffer();

// ── Double-buffer / diff-flush (used by games) ────────────────────────────────
// Call beginFrame() before any gfx draw calls for one logical frame.
// All gfx-library writes are silently redirected to a PSRAM back buffer.
// Call endFrame() when the frame is complete: it compares the back
// buffer against the display framebuffer row-by-row, copies only rows that
// changed, flushes the CPU D-cache to PSRAM in contiguous batches, then
// restores gfx to the real display framebuffer.
void beginFrame();
void endFrame();

}  // namespace display
}  // namespace sys

// Convenience: global gfx pointer for drawing (points to display::gfx).
extern Arduino_GFX *gfx;
