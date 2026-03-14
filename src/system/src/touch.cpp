#include "touch.h"
#include <Wire.h>
#include "Touch_GT911.h"

namespace sys {
namespace touch {

static Touch_GT911 _ts(19, 45, -1, -1, 480, 480);

void init() {
    Wire.begin(19, 45);
    _ts.begin();
    _ts.setRotation(ROTATION_NORMAL);
}

bool getTouch(int &x, int &y) {
    _ts.read();
    if (_ts.isTouched && _ts.touches > 0) {
        x = 479 - _ts.points[0].x;
        y = 479 - _ts.points[0].y;
        return true;
    }
    return false;
}

}  // namespace touch
}  // namespace sys
