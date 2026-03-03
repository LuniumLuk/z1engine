#pragma once

#include "core/core.h"
#include "core/maths.h"
#include "event/key_event.h"
#include "event/mouse_event.h"
#include "glm/glm.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "scene/component/base.h"

namespace z1 {

	REFLECTED_STRUCT(CameraComponent) : Requires<TransformComponent> {
		CameraComponent() = default;

		DISABLE_COPY(CameraComponent)

		glm::mat4 get_proj() const {
			if (m_is_perspective) {
				return glm::perspective(glm::radians(m_intrinsic.fov), m_aspect, m_near, m_far);
			}
			else {
				float half_height = m_intrinsic.size / 2.0f;
				float half_width = half_height * m_aspect;
				return glm::ortho(-half_width, half_width, -half_height, half_height, m_near, m_far);
			}
		}

		glm::mat4 get_view() const {
			auto& transform = get_component<TransformComponent>();
			auto world_transform = transform.get_world_transform();

			auto up = world_transform * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
			auto forward = world_transform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
			return glm::lookAt(transform.m_location, transform.m_location + glm::vec3(forward), glm::vec3(up));
		}

		glm::vec3 get_position() const {
			auto& transform = get_component<TransformComponent>();
			return transform.m_location;
		}

		void zoom(float zoom) {
			if (!(zoom > 0.0f) || std::isnan(zoom) || std::isinf(zoom)) {
				CORE_ERROR("invalid zoom factor: {}", zoom);
				return;
			}

			constexpr float epsilon = 1e-6f; // lower bound
			zoom = std::max(zoom, epsilon);

			if (m_is_perspective) {
				m_intrinsic.fov /= zoom;
				// keep fov in a meaningful range
				m_intrinsic.fov = std::clamp(m_intrinsic.fov, 1.0f, 179.0f);
			}
			else {
				m_intrinsic.size /= zoom;
				// prevent collapse or explosion of size
				m_intrinsic.size = std::clamp(m_intrinsic.size, epsilon, 1e6f);
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

	REFLECTED_FIELD(CameraComponent, m_is_perspective,   FF_Default)
	REFLECTED_FIELD(CameraComponent, m_near,             FF_Default, "[slider]")
	REFLECTED_FIELD(CameraComponent, m_far,              FF_Default, "[slider]")
	REFLECTED_FIELD(CameraComponent, m_aspect,           FF_ReadOnly)
	REFLECTED_FIELD(CameraComponent, m_use_fixed_aspect, FF_Default)
	REFLECTED_FIELD(CameraComponent, m_is_primary,       FF_ReadOnly)

}
