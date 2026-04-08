#include "sketch.h"

#include "system/display.h"
#include "system/fs.h"
#include "system/touch.h"

#include <Arduino.h>

namespace apps::sketch {

namespace {

constexpr int W = SCREEN_W;
constexpr int H = SCREEN_H;
constexpr int TOP_H = 56;
constexpr int BOTTOM_H = 56;
constexpr int CANVAS_Y0 = TOP_H;
constexpr int CANVAS_Y1 = H - BOTTOM_H;

constexpr uint16_t COL_BG = 0xDEFB;
constexpr uint16_t COL_BAR = 0x2145;
constexpr uint16_t COL_BTN = 0x4A69;
constexpr uint16_t COL_BTN_BORDER = 0xFFFF;
constexpr uint16_t COL_TEXT = 0xFFFF;

constexpr uint16_t PALETTE[] = {
    0x0000, // black
    0xFFFF, // white
    0xF800, // red
    0x07E0, // green
    0x001F, // blue
    0xFFE0, // yellow
    0xF81F, // magenta
    0x07FF  // cyan
};
constexpr int PALETTE_N = (int)(sizeof(PALETTE) / sizeof(PALETTE[0]));

constexpr int BRUSH_R = 4;

constexpr int BTN_W = 98;
constexpr int BTN_H = 40;
constexpr int BTN_Y = 8;
constexpr int BTN_CLEAR_X = W - BTN_W * 2 - 18;
constexpr int BTN_EXIT_X = W - BTN_W - 10;

constexpr int PAL_D = 30;
constexpr int PAL_GAP = 8;
constexpr int PAL_Y = H - BOTTOM_H + (BOTTOM_H - PAL_D) / 2;
constexpr int PAL_X0 = 12;

constexpr char sketch_file[] = "/sketch.bin";
constexpr size_t frame_bytes = (size_t)W * H * sizeof(uint16_t);

enum class TouchAction { NONE, DRAW, CLEAR, EXIT, COLOR_PICK };

bool inRect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && x < (rx + rw) && y >= ry && y < (ry + rh);
}

int colorHit(int x, int y) {
    for (int i = 0; i < PALETTE_N; i++) {
        const int sx = PAL_X0 + i * (PAL_D + PAL_GAP);
        if (inRect(x, y, sx, PAL_Y, PAL_D, PAL_D)) return i;
    }
    return -1;
}

void drawTopBar() {
    gfx->fillRect(0, 0, W, TOP_H, COL_BAR);
    gfx->drawFastHLine(0, TOP_H - 1, W, COL_BTN_BORDER);

    gfx->setTextColor(COL_TEXT);
    gfx->setTextSize(2);
    gfx->setCursor(12, 20);
    gfx->print("Sketch");

    gfx->fillRoundRect(BTN_CLEAR_X, BTN_Y, BTN_W, BTN_H, 8, COL_BTN);
    gfx->drawRoundRect(BTN_CLEAR_X, BTN_Y, BTN_W, BTN_H, 8, COL_BTN_BORDER);
    gfx->setCursor(BTN_CLEAR_X + 18, BTN_Y + 12);
    gfx->print("CLEAR");

    gfx->fillRoundRect(BTN_EXIT_X, BTN_Y, BTN_W, BTN_H, 8, COL_BTN);
    gfx->drawRoundRect(BTN_EXIT_X, BTN_Y, BTN_W, BTN_H, 8, COL_BTN_BORDER);
    gfx->setCursor(BTN_EXIT_X + 28, BTN_Y + 12);
    gfx->print("EXIT");
}

void drawBottomBar(int selected_color) {
    gfx->fillRect(0, H - BOTTOM_H, W, BOTTOM_H, COL_BAR);
    gfx->drawFastHLine(0, H - BOTTOM_H, W, COL_BTN_BORDER);

    for (int i = 0; i < PALETTE_N; i++) {
        const int sx = PAL_X0 + i * (PAL_D + PAL_GAP);
        gfx->fillCircle(sx + PAL_D / 2, PAL_Y + PAL_D / 2, PAL_D / 2, PALETTE[i]);
        if (i == selected_color) {
            gfx->drawCircle(sx + PAL_D / 2, PAL_Y + PAL_D / 2, PAL_D / 2 + 2, COL_BTN_BORDER);
            gfx->drawCircle(sx + PAL_D / 2, PAL_Y + PAL_D / 2, PAL_D / 2 + 3, COL_BTN_BORDER);
        }
    }
}

void clearCanvas(uint16_t *fb) {
    for (int y = CANVAS_Y0; y < CANVAS_Y1; y++) {
        uint16_t *row = fb + y * W;
        for (int x = 0; x < W; x++) row[x] = COL_BG;
    }
}

void saveCanvas(uint16_t *fb) {
    if (!sys::fs::is_mounted) return;
    sys::fs::writeWholeFile(sketch_file, fb, frame_bytes);
}

void loadCanvas(uint16_t *fb) {
    bool loaded = false;
    if (sys::fs::is_mounted) {
        loaded = sys::fs::readWholeFile(sketch_file, fb, frame_bytes);
    }
    if (!loaded) clearCanvas(fb);
}

void drawBrushDot(int x, int y, uint16_t color) {
    if (x < 0 || x >= W || y < CANVAS_Y0 || y >= CANVAS_Y1) return;
    gfx->fillCircle(x, y, BRUSH_R, color);
}

void drawThickLine(int x0, int y0, int x1, int y1, uint16_t color) {
    const int dx = x1 - x0;
    const int dy = y1 - y0;
    const int adx = dx < 0 ? -dx : dx;
    const int ady = dy < 0 ? -dy : dy;
    const int steps = adx > ady ? adx : ady;
    if (steps == 0) {
        drawBrushDot(x0, y0, color);
        return;
    }
    for (int i = 0; i <= steps; i++) {
        const int x = x0 + (dx * i) / steps;
        const int y = y0 + (dy * i) / steps;
        drawBrushDot(x, y, color);
    }
}

}  // namespace

void run() {
    uint16_t *fb = sys::display::getBufferedFramebuffer();
    int selected_color = 0;

    loadCanvas(fb);
    drawTopBar();
    drawBottomBar(selected_color);
    sys::display::flush();

    bool had_touch = false;
    sys::touch::Point prev = {0, 0};
    TouchAction action = TouchAction::NONE;
    int picked_color = -1;

    while (true) {
        fb = sys::display::getBufferedFramebuffer();
        sys::touch::Point t = {0, 0};
        const bool touching = sys::touch::getTouch(t);

        if (touching && !had_touch) {
            picked_color = colorHit(t.x, t.y);
            if (inRect(t.x, t.y, BTN_CLEAR_X, BTN_Y, BTN_W, BTN_H)) {
                action = TouchAction::CLEAR;
            } else if (inRect(t.x, t.y, BTN_EXIT_X, BTN_Y, BTN_W, BTN_H)) {
                action = TouchAction::EXIT;
            } else if (picked_color >= 0) {
                action = TouchAction::COLOR_PICK;
            } else if (t.y >= CANVAS_Y0 && t.y < CANVAS_Y1) {
                action = TouchAction::DRAW;
                drawBrushDot(t.x, t.y, PALETTE[selected_color]);
                sys::display::flush();
            } else {
                action = TouchAction::NONE;
            }
            prev = t;
        } else if (touching) {
            if (action == TouchAction::DRAW &&
                prev.y >= CANVAS_Y0 && prev.y < CANVAS_Y1 &&
                t.y >= CANVAS_Y0 && t.y < CANVAS_Y1) {
                drawThickLine(prev.x, prev.y, t.x, t.y, PALETTE[selected_color]);
                sys::display::flush();
            }
            prev = t;
        } else if (!touching && had_touch) {
            if (action == TouchAction::CLEAR) {
                clearCanvas(fb);
                drawTopBar();
                drawBottomBar(selected_color);
                sys::display::flush();
            } else if (action == TouchAction::EXIT) {
                saveCanvas(fb);
                return;
            } else if (action == TouchAction::COLOR_PICK && picked_color >= 0) {
                selected_color = picked_color;
                drawBottomBar(selected_color);
                sys::display::flush();
            }
            action = TouchAction::NONE;
            picked_color = -1;
        }

        had_touch = touching;
        delay(8);
    }
}

}  // namespace apps::sketch

