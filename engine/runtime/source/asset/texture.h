#pragma once

#include "core/core.h"
#include "core/guid.h"
#include "asset/asset.h"
#include "render/image.h"

namespace z1 {

	struct API Texture2D : Asset<Texture2D> {

		Texture2D(std::shared_ptr<Image2D> const& image, SamplerMode sampler_mode = SamplerMode::Linear, WrapMode wrap_mode = WrapMode::Repeat)
			: m_image(image)
			, m_sampler_mode(sampler_mode)
			, m_wrap_mode(wrap_mode) {
		}

		// --- begin asset interface ---
		static std::shared_ptr<Texture2D> load(Guid const& guid, AssetMeta const& meta, Filepath const& file);
		// --- end asset interface ---
		static std::shared_ptr<Texture2D> create_plain_color(glm::vec4 const& color);

		SamplerMode m_sampler_mode = SamplerMode::Linear;
		WrapMode m_wrap_mode = WrapMode::Repeat;

		std::shared_ptr<Image2D> m_image;

	};

	struct API SubTexture2D : Asset<SubTexture2D> {
		SubTexture2D(std::shared_ptr<Texture2D> texture, glm::vec2 const& min, glm::vec2 const& max)
			: m_texture(texture) {
			m_texcoords[0] = min;
			m_texcoords[1] = glm::vec2(max.x, min.y);
			m_texcoords[2] = max;
			m_texcoords[3] = glm::vec2(min.x, max.y);
		}

		std::array<glm::vec2, 4> get_texcoords() const { return m_texcoords; }

		static std::shared_ptr<SubTexture2D> create(
			std::shared_ptr<Texture2D> texture,
			uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool top_left_origion = false) {
			if (top_left_origion) {
				y = texture->m_image->get_description().m_height - y - height;
			}
			return std::make_shared<SubTexture2D>(texture,
				glm::vec2(
					(float)x / texture->m_image->get_description().m_width,
					(float)y / texture->m_image->get_description().m_height
				),
				glm::vec2(
					(float)(x + width) / texture->m_image->get_description().m_width,
					(float)(y + height) / texture->m_image->get_description().m_height
				));
		}

		// --- begin asset interface ---
		static std::shared_ptr<SubTexture2D> create(Filepath const& path, std::shared_ptr<Texture2D> texture, glm::vec2 const& min, glm::vec2 const& max) {
			UNIMPLEMENTED_FUNCTION();
			return nullptr;
		}
		static std::shared_ptr<SubTexture2D> load(Guid const& guid, AssetMeta const& meta, Filepath const& file) {
			UNIMPLEMENTED_FUNCTION();
			return nullptr;
		}
		void save() const {
			UNIMPLEMENTED_FUNCTION();
		}
		// --- end asset interface ---

		std::array<glm::vec2, 4> m_texcoords;
		std::shared_ptr<Texture2D> m_texture;
	};

}
