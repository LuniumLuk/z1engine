#include "pch.h"
#include "core/core.h"
#include "core/input.h"
#include "core/window.h"
#include "glfw/glfw3.h"

namespace z1 {

    bool InputSystem::is_key_pressed(int keycode) {
        auto window = static_cast<GLFWwindow*>(g_runtime_context.m_window->get_native_window());
        auto state = glfwGetKey(window, keycode);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool InputSystem::is_mouse_button_pressed(int button) {
        auto window = static_cast<GLFWwindow*>(g_runtime_context.m_window->get_native_window());
        auto state = glfwGetMouseButton(window, button);
        return state == GLFW_PRESS;
    }

    std::pair<float, float> InputSystem::get_mouse_pos() {
        auto window = static_cast<GLFWwindow*>(g_runtime_context.m_window->get_native_window());
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        return { (float)x, (float)y };
    }

}
