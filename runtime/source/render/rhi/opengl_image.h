#pragma once

#include "render/image.h"
#include "glad/glad.h"

namespace z1 {

	GLenum image_format_to_opengl_internal_format(ImageFormat format);

	GLenum image_format_to_opengl_format(ImageFormat format);

	GLenum image_format_to_opengl_data_type(ImageFormat format);

	GLenum sampler_mode_to_opengl_type(SamplerMode mode);

	GLenum sampler_mode_to_opengl_mipmap_type(SamplerMode mode);

	GLenum wrap_mode_to_opengl_type(WrapMode mode);

	size_t image_format_to_opengl_data_size(ImageFormat format);

	struct OpenGLImage2D : Image2D {
		friend struct OpenGLFramebuffer;

		OpenGLImage2D(void const* data, size_t size, Description const& desc);
		~OpenGLImage2D() override;

		void bind(uint32_t binding) const override;
		void unbind(uint32_t binding) const override;
		void write(void const* data, size_t size) const override;

		void* get_native_handle() const override { return (void*)(uint64_t)m_handle; }

	private:
		uint32_t m_handle = 0;
	};

	struct OpenGLImageCube : ImageCube {
		OpenGLImageCube(Faces data, size_t size, Description const& desc);
		~OpenGLImageCube() override;

		void bind(uint32_t binding) const override;
		void unbind(uint32_t binding) const override;
		void write(void const* data, size_t size) const override;

		void* get_native_handle() const override { return (void*)(uint64_t)m_handle; }

	private:
		uint32_t m_handle = 0;
	};

}
