#include "gfx.hpp"
#include "system/globals.h"
#include <esp_err.h>
#include <esp_lcd_panel_rgb.h>

namespace sys::display {

GFX::GFX(esp_lcd_panel_handle_t panel_handle) :
    Arduino_RGB_Display(
        SCREEN_W, SCREEN_H, nullptr, 0, false,
        nullptr, GFX_NOT_DEFINED,
        nullptr, 0, 0, 0, 0, 0),
    panel_handle(panel_handle) {
}

GFX::~GFX () {
    // send spi command to turn off?

    // LCD_BK - sync with display.cpp; maybe move init and spi to class?
    constexpr gpio_num_t LCD_BK = GPIO_NUM_38; 
    gpio_set_level(LCD_BK, HIGH);
    esp_lcd_panel_del(panel_handle);
}

bool GFX::begin(int32_t speed) {
    ESP_ERROR_CHECK(
        esp_lcd_rgb_panel_get_frame_buffer(
            panel_handle,
            2,
            (void **)&fb1,
            (void **)&fb2
        )
    );
    _framebuffer = fb1;

    esp_lcd_rgb_panel_event_callbacks_t callbacks {
        .on_vsync = [](esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx) -> bool {
            return ((GFX*)user_ctx)->vsyncCallback();
        }
    };

    esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &callbacks, this);

    render_task_handle = xTaskGetCurrentTaskHandle();

    return true;
} 

bool GFX::startFrame(bool block) {
    uint32_t _;
    BaseType_t result = xTaskGenericNotifyWait(MainTaskRenderNotifyIndex, 0, UINT32_MAX, &_, block ? portMAX_DELAY : 0);
    return result == pdTRUE;
}

void GFX::endFrame() {
    pending_sync = true;
}

void GFX::flush(bool force_flush) {
    endWrite();
}

// === mock
void GFX::writePixelPreclipped(int16_t x, int16_t y, uint16_t color) {
    _framebuffer[x + y * _width] = color;
}
void GFX::startWrite() {}
void GFX::endWrite() {}

bool GFX::vsyncCallback() {
    if (pending_sync) {
        swapBuffers(); 
        pending_sync = false;
    }
    
    BaseType_t task_wakeup = pdFALSE;
    xTaskGenericNotify(render_task_handle, MainTaskRenderNotifyIndex, task_wakeup, eSetValueWithOverwrite, NULL);
    return task_wakeup == pdTRUE;
}

// === getters

uint16_t *GFX::getCurrentFramebuffer() {
    return _framebuffer;
}

uint16_t *GFX::getFirstFramebuffers() {
    return fb1;
}

uint16_t *GFX::getSecondFramebuffers() {
    return fb2;
}

// === private

void GFX::swapBuffers() {
    // safe to use in callback coz it just swaps pointers
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, WIDTH, HEIGHT, _framebuffer);

    if (_framebuffer == fb1) {
        _framebuffer = fb2;
    }
    else {
        _framebuffer = fb2;
    }
}

} // namespace sys::display
