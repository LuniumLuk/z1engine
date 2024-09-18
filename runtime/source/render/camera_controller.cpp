#include "pch.h"
#include "render/camera_controller.h"
#include "core/core.h"
#include "core/keycodes.h"
#include "core/input.h"

namespace z1 {

    void InputState::on_event(Event& event) {
        auto dispatcher = EventDispatcher(event);
        dispatcher.dispatch<MouseScrollEvent>(BIND_EVENT_FN(InputState::on_mouse_scrolled));
        dispatcher.dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(InputState::on_mouse_pressed));
        dispatcher.dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN(InputState::on_mouse_released));
    }

    void InputState::update(double delta_time) {
        m_delta_time = delta_time;

        auto [mouse_x, mouse_y] = g_runtime_context.m_input_system->get_mouse_pos();

        m_mouse_delta_x = mouse_x - m_mouse_last_x;
        m_mouse_delta_y = mouse_y - m_mouse_last_y;
        m_mouse_last_x = mouse_x;
        m_mouse_last_y = mouse_y;
    }

    void InputState::reset() {
        m_delta_time = 0.0;
        m_mouse_delta_x = 0.0f;
        m_mouse_delta_y = 0.0f;
        m_scroll_delta = 0.0f;
    }

    bool InputState::on_mouse_scrolled(MouseScrollEvent& event) {
        m_scroll_delta += event.get_y_offset();
        return false;
    }

    bool InputState::on_mouse_pressed(MouseButtonPressedEvent& event) {
        if (event.GetButton() == MOUSE_BUTTON_RIGHT) {
            m_right_button_pressed = true;
        }
        if (event.GetButton() == MOUSE_BUTTON_LEFT) {
            m_left_button_pressed = true;
        }
        return true;
    }

    bool InputState::on_mouse_released(MouseButtonReleasedEvent& event) {
        if (event.GetButton() == MOUSE_BUTTON_RIGHT) {
            m_right_button_pressed = false;
        }
        if (event.GetButton() == MOUSE_BUTTON_LEFT) {
            m_left_button_pressed = false;
        }
        return true;
    }

    CameraController::CameraController(std::shared_ptr<Camera> const& camera)
        : m_camera(camera) {}

    void HoveringCameraController::update(InputState const& state) {
        PROFILE_FUNCTION();
        if (state.m_right_button_pressed) {
            m_camera->rotate(
                state.m_mouse_delta_y * m_rotate_speed * (float)state.m_delta_time,
                state.m_mouse_delta_x * m_rotate_speed * (float)state.m_delta_time,
                0.0f);
        }

        glm::vec3 move(0.0f);
        if (g_runtime_context.m_input_system->is_key_pressed(KEY_A)) {
            move.x -= m_move_speed * (float)state.m_delta_time;
        }
        if (g_runtime_context.m_input_system->is_key_pressed(KEY_D)) {
            move.x += m_move_speed * (float)state.m_delta_time;
        }
        if (g_runtime_context.m_input_system->is_key_pressed(KEY_S)) {
            move.z -= m_move_speed * (float)state.m_delta_time;
        }
        if (g_runtime_context.m_input_system->is_key_pressed(KEY_W)) {
            move.z += m_move_speed * (float)state.m_delta_time;
        }

        m_camera->move(move);
    }

    void OrbitingCameraController::update(InputState const& state) {
        PROFILE_FUNCTION();
        UNIMPLEMENTED_FUNCTION();
    }

    void Generic2DCameraController::update(InputState const& state) {
        PROFILE_FUNCTION();
        glm::vec3 move(0.0f);
        if (g_runtime_context.m_input_system->is_key_pressed(KEY_A)) {
            move.x -= m_move_speed * (float)state.m_delta_time;
        }
        if (g_runtime_context.m_input_system->is_key_pressed(KEY_D)) {
            move.x += m_move_speed * (float)state.m_delta_time;
        }
        if (g_runtime_context.m_input_system->is_key_pressed(KEY_S)) {
            move.y -= m_move_speed * (float)state.m_delta_time;
        }
        if (g_runtime_context.m_input_system->is_key_pressed(KEY_W)) {
            move.y += m_move_speed * (float)state.m_delta_time;
        }

        if (state.m_left_button_pressed) {
            move.x -= m_drag_speed * state.m_mouse_delta_x * (float)state.m_delta_time;
            move.y += m_drag_speed * state.m_mouse_delta_y * (float)state.m_delta_time;
        }

        m_camera->move(move);
        m_camera->zoom(std::exp(m_zoom_speed * state.m_scroll_delta * (float)state.m_delta_time));
    }

}
