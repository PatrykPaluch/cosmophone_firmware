#pragma once

namespace sys {
namespace menu {

enum class AppMode {
    SNAKE,
    FLAPPY,
    RAINBOW,
};

// Blocks until the user selects an item, then returns the chosen mode.
AppMode showMenu();

}  // namespace menu
}  // namespace sys
