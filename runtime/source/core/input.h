#pragma once

#include "core/core.h"

namespace z1 {

    struct API InputSystem {
        bool is_key_pressed(int keycode);
        bool is_mouse_button_pressed(int button);
        std::pair<float, float> get_mouse_pos();
    };

}
