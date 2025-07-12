#pragma once

#include "core/io.h"
#include "render/image.h"

namespace z1::io {

	bool file_is_ldr_image(Filepath const& path) noexcept;
	bool file_is_hdr_image(Filepath const& path) noexcept;

	std::shared_ptr<Image2D> load_image2d(
		Filepath const& path,
		SamplerMode sampler_mode = SamplerMode::Linear,
		WrapMode wrap_mode = WrapMode::Repeat);

}
