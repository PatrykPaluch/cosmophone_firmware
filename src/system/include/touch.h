#pragma once

namespace sys {
namespace touch {

void init();

// Returns true and sets x, y (screen coordinates) when the screen is touched.
bool getTouch(int &x, int &y);

}  // namespace touch
}  // namespace sys
