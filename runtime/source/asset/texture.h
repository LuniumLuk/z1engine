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

		static std::shared_ptr<Texture2D> load(Guid const& guid);

		SamplerMode m_sampler_mode = SamplerMode::Linear;
		WrapMode m_wrap_mode = WrapMode::Repeat;

		std::shared_ptr<Image2D> m_image;

	};

}
