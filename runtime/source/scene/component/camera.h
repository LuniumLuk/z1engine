#pragma once

#include "core/core.h"
#include "event/key_event.h"
#include "event/mouse_event.h"
#include "glm/glm.hpp"
#include "glm/ext/matrix_clip_space.hpp"

namespace z1 {

	struct API CameraComponent {
		CameraComponent() = default;

		CameraComponent(CameraComponent const&) = default;
		CameraComponent& operator=(CameraComponent const&) = default;
		CameraComponent(CameraComponent&&) = delete;
		CameraComponent& operator=(CameraComponent&&) = delete;

		~CameraComponent() = default;

		glm::mat4 get_proj() const {
			if (m_is_perspective) {
				return glm::perspective(glm::radians(m_intrinsic.fov), m_aspect, m_near, m_far);
			}
			else {
				float half_width = m_intrinsic.size / 2.0f;
				float half_height = half_width * m_aspect;
				return glm::ortho(-half_width, half_width, -half_height, half_height, m_near, m_far);
			}
		}

		bool m_is_perspective = true; // true if the camera is a perspective camera, false if it is an orthographic camera

		union {
			float fov = 45.f;  // field of view in degree, only used for perspective camera
			float size;        // size of the orthographic frustum, only used for orthographic camera
		} m_intrinsic;

		float m_near = 0.01f;
		float m_far = 1000.0f;
		float m_aspect = 1.0f;

		bool m_use_fixed_aspect = false; // if true, the aspect ratio will not change when the window size changes
		bool m_is_primary = false;       // if true, this camera will be used as the primary camera for rendering
	};

}
