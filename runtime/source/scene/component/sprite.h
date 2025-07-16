#pragma once

#include "core/core.h"
#include "render/image.h"
#include "glm/glm.hpp"

namespace z1 {

	struct API SpriteComponent {
		glm::vec4 m_color{ 1.0f, 1.0f, 1.0f, 1.0f };
		std::shared_ptr<Image2D> m_texture = nullptr;
		glm::vec2 m_tiling_scale = glm::vec2(1.0f);
		glm::vec2 m_tiling_offset = glm::vec2(0.0f);
		std::array<glm::vec2, 4> m_texcoords = { {
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f },
		} };

		SpriteComponent() = default;
		SpriteComponent(glm::vec4 const& color) noexcept
			: m_color(color) {
		}
		SpriteComponent(glm::vec4 const& color, std::shared_ptr<Image2D> const& texture) noexcept
			: m_color(color)
			, m_texture(texture) {
		}
		SpriteComponent(glm::vec4 const& color, std::shared_ptr<SubImage2D> const& texture) noexcept
			: m_color(color)
			, m_texture(texture->m_image)
			, m_texcoords(texture->m_texcoords) {
		}
	};

}
