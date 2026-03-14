#include "menu.h"
#include "display.h"
#include "touch.h"

namespace sys {
namespace menu {

// ── Palette ───────────────────────────────────────────────────────────────────
static const uint16_t M_BG      = 0x0841;  // very dark blue-grey
static const uint16_t M_TITLE   = 0x07FF;  // cyan
static const uint16_t M_SUB     = 0x7BEF;  // light grey
static const uint16_t M_BTN     = 0x2145;  // dark button
static const uint16_t M_BTN_HL  = 0x4A6F;  // highlighted button
static const uint16_t M_BTN_BDR = 0x07FF;  // cyan border
static const uint16_t M_TEXT    = 0xFFFF;  // white

// ── Menu item descriptor ──────────────────────────────────────────────────────
struct MenuItem {
    const char  *label;
    const char  *subtitle;
    uint16_t     iconColor;
    AppMode      mode;
};

static const MenuItem ITEMS[] = {
    { "SNAKE",       "Classic snake game",       0x07E0, AppMode::SNAKE       },
    { "FLAPPY BIRD", "Tap to flap!",             0xFFE0, AppMode::FLAPPY      },
    { "RAINBOW",     "Scrolling rainbow demo",   0xF81F, AppMode::RAINBOW     },
};
static const int N_ITEMS = sizeof(ITEMS) / sizeof(ITEMS[0]);

// ── Layout ────────────────────────────────────────────────────────────────────
static const int HEADER_H    = 90;
static const int BTN_W       = 400;
static const int BTN_H       = 70;
static const int BTN_X       = (480 - BTN_W) / 2;
static const int BTN_GAP     = 10;
static const int BTN_START_Y = HEADER_H + 10;

// ── Drawing ───────────────────────────────────────────────────────────────────
static void drawHeader() {
    gfx->fillRect(0, 0, 480, HEADER_H, 0x1082);
    gfx->drawFastHLine(0, HEADER_H, 480, M_TITLE);

    gfx->setTextColor(M_TITLE);
    gfx->setTextSize(4);
    gfx->setCursor((480 - 10 * 24) / 2, 12);
    gfx->print("COSMOPHONE");

    gfx->setTextColor(M_SUB);
    gfx->setTextSize(2);
    gfx->setCursor((480 - 12 * 12) / 2, 58);
    gfx->print("Game Console");
}

static void drawItem(int idx, bool highlighted) {
    const MenuItem &item = ITEMS[idx];
    int y = BTN_START_Y + idx * (BTN_H + BTN_GAP);

    uint16_t bg  = highlighted ? M_BTN_HL : M_BTN;
    gfx->fillRoundRect(BTN_X, y, BTN_W, BTN_H, 14, bg);
    gfx->drawRoundRect(BTN_X, y, BTN_W, BTN_H, 14, M_BTN_BDR);

    gfx->fillRoundRect(BTN_X + 12, y + 10, 50, 50, 8, item.iconColor);

    gfx->setTextColor(M_TEXT);
    gfx->setTextSize(3);
    gfx->setCursor(BTN_X + 78, y + 10);
    gfx->print(item.label);

    gfx->setTextColor(M_SUB);
    gfx->setTextSize(1);
    gfx->setCursor(BTN_X + 80, y + 48);
    gfx->print(item.subtitle);
}

static void drawMenu() {
    gfx->fillScreen(M_BG);
    drawHeader();
    for (int i = 0; i < N_ITEMS; i++) drawItem(i, false);

    gfx->setTextColor(0x4208);
    gfx->setTextSize(1);
    gfx->setCursor(4, 472);
    gfx->print("v1.0  |  Touch a game to launch");
}

AppMode showMenu() {
    drawMenu();

    while (true) {
        int tx, ty;
        if (!sys::touch::getTouch(tx, ty)) { delay(10); continue; }

        for (int i = 0; i < N_ITEMS; i++) {
            int y = BTN_START_Y + i * (BTN_H + BTN_GAP);
            if (tx >= BTN_X && tx < BTN_X + BTN_W &&
                ty >= y    && ty < y + BTN_H) {
                drawItem(i, true);
                delay(120);
                return ITEMS[i].mode;
            }
        }
        delay(10);
    }
}

}  // namespace menu
}  // namespace sys
