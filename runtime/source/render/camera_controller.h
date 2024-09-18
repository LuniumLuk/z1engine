#pragma once

#include "core/core.h"
#include "event/key_event.h"
#include "event/mouse_event.h"
#include "render/camera.h"

namespace z1 {

    struct API InputState {
        bool m_right_button_pressed = false;
        bool m_left_button_pressed = false;
        float m_scroll_delta = 0.0f;
        float m_mouse_delta_x = 0.0f, m_mouse_delta_y = 0.0f;
        float m_mouse_last_x = 0.0f, m_mouse_last_y = 0.0f;
        double m_delta_time;

        void on_event(Event& event);
        void update(double delta_time);
        void reset();

    private:
        bool on_mouse_scrolled(MouseScrollEvent& event);
        bool on_mouse_pressed(MouseButtonPressedEvent& event);
        bool on_mouse_released(MouseButtonReleasedEvent& event);
    };

    struct API CameraController {
        CameraController(std::shared_ptr<Camera> const& camera);

        virtual void update(InputState const& state) = 0;

        std::shared_ptr<Camera> const& GetCamera() const { return m_camera; }

    protected:
        std::shared_ptr<Camera> m_camera;
    };

    struct API HoveringCameraController : CameraController {
        HoveringCameraController(std::shared_ptr<Camera> const& camera)
            : CameraController(camera) {}
        
        void update(InputState const& state) override;

        float m_rotate_speed = 1.0f;
        float m_move_speed = 1.0f;
    };

    struct API OrbitingCameraController : CameraController {
        OrbitingCameraController(std::shared_ptr<Camera> const& camera)
            : CameraController(camera) {}

        void update(InputState const& state) override;
    };

    struct API Generic2DCameraController : CameraController {
        Generic2DCameraController(std::shared_ptr<Camera> const& camera)
            : CameraController(camera) {}

        void update(InputState const& state) override;

        float m_drag_speed = 1.0f;
        float m_move_speed = 1.0f;
        float m_zoom_speed = 1.0f;
    };

}
