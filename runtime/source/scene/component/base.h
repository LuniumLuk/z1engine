#pragma once

#include "core/core.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

namespace z1 {

	struct API TagComponent {
		std::string m_tag;
		TagComponent() = default;
		TagComponent(std::string const& tag) : m_tag(tag) {}
	};

	struct API TransformComponent {
		glm::vec3 m_location{ 0.0f, 0.0f, 0.0f };
		glm::vec3 m_rotation{ 0.0f, 0.0f, 0.0f }; // in degrees (pitch, yaw, roll)
		glm::vec3 m_scale{ 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(glm::vec3 const& location, glm::vec3 const& rotation, glm::vec3 const& scale) noexcept
			: m_location(location), m_rotation(rotation), m_scale(scale) {
		}

		glm::mat4 get_transform() const {
			glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_location);
			glm::mat4 rotation = get_rotation();
			glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_scale);
			return translation * rotation * scale;
		}

		glm::mat4 get_rotation() const {
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.y), { 0.0f, 1.0f, 0.0f }); // yaw
			rotation = glm::rotate(rotation, glm::radians(m_rotation.x), { 1.0f, 0.0f, 0.0f }); // pitch
			rotation = glm::rotate(rotation, glm::radians(m_rotation.z), { 0.0f, 0.0f, 1.0f }); // roll
			return rotation;
		}
	};

}
