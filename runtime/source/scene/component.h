#pragma once

#include "core/core.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace z1 {

	struct API TagComponent {
		std::string m_tag;

		TagComponent() = default;
		TagComponent(std::string const& tag);

		TagComponent(TagComponent const&) = default;
		TagComponent& operator=(TagComponent const&) = default;
		TagComponent(TagComponent&&) = delete;
		TagComponent& operator=(TagComponent&&) = delete;

		~TagComponent() = default;
	};

	struct API TransformComponent {
		glm::vec3 m_location{ 0.0f, 0.0f, 0.0f };
		glm::vec3 m_rotation{ 0.0f, 0.0f, 0.0f }; // in degrees
		glm::vec3 m_scale{ 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(glm::vec3 const& location, glm::vec3 const& rotation, glm::vec3 const& scale);

		TransformComponent(TransformComponent const&) = default;
		TransformComponent& operator=(TransformComponent const&) = default;
		TransformComponent(TransformComponent&&) = delete;
		TransformComponent& operator=(TransformComponent&&) = delete;

		~TransformComponent() = default;

		glm::mat4 get_transform() const;
	};

	struct API SpriteComponent {
		glm::vec4 m_color{ 1.0f, 1.0f, 1.0f, 1.0f };

		SpriteComponent() = default;
		SpriteComponent(glm::vec4 const& color);

		SpriteComponent(SpriteComponent const&) = default;
		SpriteComponent& operator=(SpriteComponent const&) = default;
		SpriteComponent(SpriteComponent&&) = delete;
		SpriteComponent& operator=(SpriteComponent&&) = delete;

		~SpriteComponent() = default;
	};

}
