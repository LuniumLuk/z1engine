#pragma once

#include "core/io.h"
#include "render/image.h"

namespace z1::io {

	std::shared_ptr<Image2D> load_image2d_asset(Filepath const& path);
	std::shared_ptr<Image2D> load_image2d(
		Filepath const& file,
		SamplerMode sampler_mode = SamplerMode::Linear,
		WrapMode wrap_mode = WrapMode::Repeat);

}
