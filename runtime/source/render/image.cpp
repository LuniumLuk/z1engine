#include "pch.h"
#include "core/core.h"
#include "render/image.h"
#include "render/shader.h"
#include "render/graphics_context.h"
#include "render/rhi/opengl_image.h"

namespace z1 {

	void Image::bind() const {
		if (m_binding != INVALID_BINDING) {
			CORE_WARN("image is already bound to binding point {0}", m_binding);
			return;
		}
		m_binding = g_runtime_context.m_graphics_context->acquire_image_binding();
		bind(m_binding);
	}

	void Image::bind(std::shared_ptr<Shader> const& shader, std::string const& name) const {
		bind();
		shader->set_uniform(name, &m_binding);
	}

	void Image::unbind() const {
		if (m_binding == INVALID_BINDING) {
			CORE_WARN("image is not bound to any binding point");
			return;
		}
		unbind(m_binding);
		g_runtime_context.m_graphics_context->release_image_binding(m_binding);
		m_binding = INVALID_BINDING;
	}

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
