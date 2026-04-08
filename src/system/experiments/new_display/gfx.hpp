#pragma once

#include <display/Arduino_RGB_Display.h>

namespace sys::display {

class GFX final : public Arduino_RGB_Display {
public:
    GFX(esp_lcd_panel_handle_t panel_handle);
    ~GFX();

    bool begin(int32_t speed = GFX_NOT_DEFINED) override;

    /**
     * @brief Begins frame drawing
     * @param[in] block if true then will block waiting for vsync
     * @return true if block==true or if vsync occurred since last startFrame() call. false otherwise
     */
    bool startFrame(bool block = true); 
    void endFrame(); // finishes frame and signals to swap buffers (draw frame on screen)

    void flush(bool force_flush = false) override; // deprecated. use startFrame()

    // === mock
    void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override;
    void startWrite() override;
    void endWrite(void) override;

    // === getters
    /** @brief Buffer should be reacquired every frame, because GFX automatically swaps buffers
     * @return buffer used to render next frame
     */
    uint16_t *getCurrentFramebuffer(); 
    uint16_t *getFirstFramebuffers(); // use getCurrentFramebuffer for rendering
    uint16_t *getSecondFramebuffers(); // use getCurrentFramebuffer for rendering 

private:
    bool vsyncCallback();
    void swapBuffers();

    esp_lcd_panel_handle_t panel_handle;
    uint16_t *fb1;
    uint16_t *fb2;
    TaskHandle_t render_task_handle = nullptr;
    volatile bool pending_sync = false;

}; // class GFX 

} // namespace sys::display 
