#include <Arduino.h>
#include "display.h"
#include "touch.h"
#include "menu.h"
#include "snake.h"
#include "flapbird.h"
#include "rainbow.h"

void setup() {
    Serial.begin(115200);
    sys::display::init();
    sys::touch::init();
}

void loop() {
    using namespace sys::menu;
    switch (showMenu()) {
        case AppMode::SNAKE:   apps::snake::run();   break;
        case AppMode::FLAPPY:  apps::flapbird::run(); break;
        case AppMode::RAINBOW: apps::rainbow::run();  break;
    }
}
