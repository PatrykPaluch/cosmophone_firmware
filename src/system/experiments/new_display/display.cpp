#include "system/display.h"
#include "gfx.hpp"
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_heap_caps.h>
#include <esp_lcd_panel_io.h>
#include <esp_err.h>
#include <databus/Arduino_SWSPI.h>
#include <display/Arduino_RGB_Display.h>

namespace {

typedef struct {
    int cmd;                
    const uint8_t *data;   
    size_t data_bytes;
} st7701_lcd_init_cmd_t;

constexpr gpio_num_t LCD_BK = GPIO_NUM_38;
constexpr gpio_num_t CS_PIN = GPIO_NUM_39;
constexpr gpio_num_t SCK_PIN = GPIO_NUM_48;
constexpr gpio_num_t MOSI_PIN = GPIO_NUM_47;
constexpr uint32_t PCLK_HZ = 16000000;

constexpr size_t st7701_typ9_init_op_length = 34;
static const st7701_lcd_init_cmd_t st7701_typ9_init_op[st7701_typ9_init_op_length] = {
//  {cmd, { data }, data_size, delay_ms}
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x10}, 5},
    {0xC0, (uint8_t[]){0x3B, 0x00}, 2},
    {0xC1, (uint8_t[]){0x0D, 0x02}, 2},
    {0xC2, (uint8_t[]){0x31, 0x05}, 2},
    {0xCD, (uint8_t[]){0x00}, 1},
    // Positive Voltage Gamma Control
    {0xB0, (uint8_t[]){0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18}, 16},
    // Negative Voltage Gamma Control
    {0xB1, (uint8_t[]){0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18}, 16},
    // PAGE1
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x11}, 5},
    {0xB0, (uint8_t[]){0x60}, 1}, // Vop=4.7375v
    {0xB1, (uint8_t[]){0x32}, 1}, // VCOM=32
    {0xB2, (uint8_t[]){0x07}, 1}, // VGH=15v
    {0xB3, (uint8_t[]){0x80}, 1},
    {0xB5, (uint8_t[]){0x49}, 1}, // VGL=-10.17v
    {0xB7, (uint8_t[]){0x85}, 1},
    {0xB8, (uint8_t[]){0x21}, 1}, // AVDD=6.6 & AVCL=-4.6
    {0xC1, (uint8_t[]){0x78}, 1},
    {0xC2, (uint8_t[]){0x78}, 1},
    
    {0xE0, (uint8_t[]){0x00, 0x1B, 0x02}, 3},
    {0xE1, (uint8_t[]){0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44}, 11},
    {0xE2, (uint8_t[]){0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00}, 12},
    {0xE3, (uint8_t[]){0x00, 0x00, 0x11, 0x11}, 4},
    {0xE4, (uint8_t[]){0x44, 0x44}, 2},
    {0xE5, (uint8_t[]){0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0}, 16},
    {0xE6, (uint8_t[]){0x00, 0x00, 0x11, 0x11}, 4},
    {0xE7, (uint8_t[]){0x44, 0x44}, 2},
    {0xE8, (uint8_t[]){0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0}, 16},
    {0xEB, (uint8_t[]){0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40}, 7},
    {0xEC, (uint8_t[]){0x3C, 0x00}, 2},
    {0xED, (uint8_t[]){0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA}, 16},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5},
    {0xE5, (uint8_t[]){0xE4}, 1},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5},
    {0x3A, (uint8_t[]){0x60}, 1},
    {0x11, (uint8_t[]){0x00}, 0},
};

static void spi_delay(){ 
    ets_delay_us(10);
}

static void spi_write9(bool command, uint8_t data)
{
    spi_delay();
    gpio_set_level(MOSI_PIN, command ? LOW : HIGH);
    spi_delay();
    gpio_set_level(SCK_PIN, HIGH);
    spi_delay();
    gpio_set_level(SCK_PIN, LOW);

    uint8_t bit = 0x80;
    while (bit)
    {
        if (data & bit) {
            gpio_set_level(MOSI_PIN, HIGH);
        }
        else {
            gpio_set_level(MOSI_PIN, LOW);
        }
        spi_delay();
        gpio_set_level(SCK_PIN, HIGH);
        bit >>= 1;
        spi_delay();
        gpio_set_level(SCK_PIN, LOW);
    }
}

void sendInitCommands() {
    gpio_set_direction(CS_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOSI_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(SCK_PIN, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(CS_PIN, GPIO_FLOATING);
    gpio_set_pull_mode(MOSI_PIN, GPIO_FLOATING);
    gpio_set_pull_mode(SCK_PIN, GPIO_FLOATING);

    gpio_set_level(CS_PIN, HIGH);
    gpio_set_level(MOSI_PIN, LOW);
    gpio_set_level(SCK_PIN, LOW);
    delay(120);

    gpio_set_level(CS_PIN, LOW);
    spi_delay();


    for (int i = 0 ; i < st7701_typ9_init_op_length; i++) {
        const st7701_lcd_init_cmd_t *cmd = &st7701_typ9_init_op[i];
        
        spi_write9(true, cmd->cmd);
        for (int j = 0 ; j < cmd->data_bytes ; j++) 
        {
            spi_write9(false, ((uint8_t*)cmd->data)[j]);
        }
        spi_delay();
    }
    gpio_set_level(CS_PIN, HIGH);
    delay(120); 

    gpio_set_level(CS_PIN, LOW);
    spi_delay();
    spi_write9(true, 0x29);
    gpio_set_level(CS_PIN, HIGH);
    delay(20);
}

} // namespace


namespace sys {
namespace display {

void init() {
    gpio_set_direction(LCD_BK, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_BK, LOW); // safety: set to low unit init is done 

    esp_lcd_rgb_panel_config_t panelConfig = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = PCLK_HZ,
            .h_res = 480,
            .v_res = 480,
      
            .hsync_pulse_width = 8, 
            .hsync_back_porch = 50, // can be optimalized
            .hsync_front_porch = 10,

            .vsync_pulse_width = 8,
            .vsync_back_porch = 20, // can be optimalized
            .vsync_front_porch = 10,
            .flags = {
                .hsync_idle_low = 0,
                .vsync_idle_low = 0,
                .de_idle_high = 0,
                .pclk_active_neg = 0,
                .pclk_idle_high = 0,
            },
        },
        .data_width = 16, // RGB565
        .bits_per_pixel = 16,
        .num_fbs = 2,
        .bounce_buffer_size_px = 480*20, // can be optimalized
        .dma_burst_size = 64,
        .hsync_gpio_num = 16,
        .vsync_gpio_num = 17,
        .de_gpio_num = 18,
        .pclk_gpio_num = 21,
        .disp_gpio_num = GPIO_NUM_NC,
        .data_gpio_nums = {
            4,  5,  6,  7,  15,
            8,  20, 3,  46, 9, 10,
            11, 12, 13, 14, 0
        },
        .flags = {
            .disp_active_low = false,
            .refresh_on_demand = false,
            .fb_in_psram = true,
            .double_fb = true,
            .no_fb = false,
            .bb_invalidate_cache = false,
        }
    };

    esp_lcd_panel_handle_t panel_handle;
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panelConfig, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    sendInitCommands();
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    gpio_set_level(LCD_BK, HIGH);
   
    GFX *rgb_display = new GFX(panel_handle);
    rgb_display->begin();

    gfx = rgb_display;
}



void drawBackButton(uint16_t bg, uint16_t border, uint16_t textColor) {}
bool backButtonTapped(int tx, int ty) {return false;}

uint16_t *getCurrentFramebuffer() { return gfx->getCurrentFramebuffer(); }

void startFrame() {
    gfx->startFrame();
}

void endFrame() {
    gfx->endFrame();
}

void flush() {
    endFrame();
}

// ==== backwards compatiblity
uint16_t *getDirectFramebuffer() {
    startFrame();
    return getCurrentFramebuffer();
}

uint16_t *getBufferedFramebuffer() {
    startFrame();
    return getCurrentFramebuffer();
}

void flushDirect() {
    flush();
}

}  // namespace display
}  // namespace sys

// Convenience: global gfx pointer for drawing.
sys::display::GFX *gfx = nullptr;