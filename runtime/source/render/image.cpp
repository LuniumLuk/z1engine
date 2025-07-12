#include "pch.h"
#include "core/core.h"
#include "render/image.h"
#include "render/rhi/opengl_image.h"

namespace z1 {

	std::shared_ptr<Image2D> Image2D::create(
		void const* data,
		size_t size,
		uint32_t width,
		uint32_t height,
		ImageFormat format,
		SamplerMode sampler_mode,
		WrapMode wrap_mode) {
		PROFILE_FUNCTION();

		Description desc{};
		desc.m_width = width;
		desc.m_height = height;
		desc.m_depth = 1;
		desc.m_format = format;
		desc.m_sampler_mode = sampler_mode;
		desc.m_wrap_mode = wrap_mode;

		return std::shared_ptr<Image2D>(new OpenGLImage2D(data, size, desc));
	}

	std::shared_ptr<ImageCube> ImageCube::create(
		Faces data,
		size_t size,
		uint32_t width,
		uint32_t height,
		ImageFormat format,
		SamplerMode sampler_mode,
		WrapMode wrap_mode) {
		PROFILE_FUNCTION();

		Description desc{};
		desc.m_width = width;
		desc.m_height = height;
		desc.m_depth = 6;
		desc.m_format = format;
		desc.m_sampler_mode = sampler_mode;
		desc.m_wrap_mode = wrap_mode;

		return std::shared_ptr<ImageCube>(new OpenGLImageCube(data, size, desc));
	}

}
