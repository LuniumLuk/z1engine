#include "pch.h"
#include "core/core.h"
#include "core/io.h"
#include "render/image.h"
#include "render/rhi/opengl_image.h"

namespace z1 {

	static ImageFormat ndarray_data_type_to_image_format(ArrayDataType type) {
		switch (type) {
		case ArrayDataType::Uint8:
			return ImageFormat::RGBA8;
		case ArrayDataType::Float32:
			return ImageFormat::RGBA32F;
		}

		CORE_ASSERT(false, "unknown ArrayDataType!");
		return ImageFormat::None;
	}

	std::shared_ptr<Image2D> Image2D::create(
		Filepath const& path,
		SamplerMode sampler_mode,
		WrapMode wrap_mode) {
		PROFILE_FUNCTION();

		auto data = g_runtime_context.m_file_system->read_image(path);
		auto format = ndarray_data_type_to_image_format(data.dtype());

		CORE_ASSERT(data.ndim() == 3, "image data must have 3 dimensions!");
		CORE_ASSERT(data.shape()[2] == 4, "image data must have 4 channels!");

		return create(
			data.data(),
			data.size(),
			data.shape()[1],
			data.shape()[0],
			format,
			sampler_mode,
			wrap_mode);
	}

	std::shared_ptr<Image2D> Image2D::create(
		void* data,
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
