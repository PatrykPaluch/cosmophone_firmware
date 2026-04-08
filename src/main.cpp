#include <Arduino.h>
#include "system/display.h"
#include "system/fs.h"
#include "system/userconfig.h"
#include "system/touch.h"
#include "system/menu.h"
#include "apps/sketch/sketch.h"
#include "apps/rainbow/rainbow.h"

void setup() {
    Serial0.begin(115200);
    Serial0.println(F("CosmoPhone: setup start"));
    Serial0.println(">>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<");
    Serial0.printf("ESP.getSdkVersion() = %s\n", ESP.getSdkVersion());
    Serial0.printf("ESP.getChipModel() = %s\n", ESP.getChipModel());
    Serial0.printf("ESP.getFreeHeap() = %d\n", ESP.getFreeHeap());
    Serial0.println(">>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<");

    delay(500);
    sys::fs::init(true);
    sys::userconfig::init();
    sys::display::init();
    sys::touch::init();
}

void loop() {
    using namespace sys::menu;
    AppMode selectedMode = showMenu();
    switch (selectedMode) {
        case AppMode::SKETCH: apps::sketch::run(); break;
        case AppMode::RAINBOW: apps::rainbow::run(); break;
        default: break;
    }
}
