#include "pch.h"
#include "render/rhi/opengl_image.h"
#include "glad/glad.h"

namespace z1 {

	// helper functions
	// --------------------------------------------------

	static GLenum image_format_to_opengl_internal_format(ImageFormat format) {
		switch (format) {
		case ImageFormat::RGBA8: return GL_RGBA8;
		case ImageFormat::RGBA32F: return GL_RGBA32F;
		case ImageFormat::Depth: return GL_DEPTH_COMPONENT24;
		case ImageFormat::DepthStencil: return GL_DEPTH24_STENCIL8;
		}
		CORE_ASSERT(false, "unknown image format!");
		return 0;
	}

	static GLenum image_format_to_opengl_format(ImageFormat format) {
		switch (format) {
		case ImageFormat::RGBA8: return GL_RGBA;
		case ImageFormat::RGBA32F: return GL_RGBA;
		case ImageFormat::Depth: return GL_DEPTH_COMPONENT;
		case ImageFormat::DepthStencil: return GL_DEPTH_STENCIL;
		}
		CORE_ASSERT(false, "unknown image format!");
		return 0;
	}

	static GLenum image_format_to_opengl_data_type(ImageFormat format) {
		switch (format) {
		case ImageFormat::RGBA8: return GL_UNSIGNED_BYTE;
		case ImageFormat::RGBA32F: return GL_FLOAT;
		case ImageFormat::Depth: return GL_FLOAT;
		case ImageFormat::DepthStencil: return GL_UNSIGNED_INT_24_8;
		}
		CORE_ASSERT(false, "unknown image format!");
		return 0;
	}

	static GLenum sampler_mode_to_opengl_type(SamplerMode mode) {
		switch (mode) {
		case SamplerMode::Linear: return GL_LINEAR;
		case SamplerMode::Nearest: return GL_NEAREST;
		}
		CORE_ASSERT(false, "unknown sampler mode!");
		return 0;
	}

	static GLenum sampler_mode_to_opengl_mipmap_type(SamplerMode mode) {
		switch (mode) {
		case SamplerMode::Linear: return GL_LINEAR_MIPMAP_LINEAR;
		case SamplerMode::Nearest: return GL_NEAREST_MIPMAP_NEAREST;
		}
		CORE_ASSERT(false, "unknown sampler mode!");
		return 0;
	}

	static GLenum wrap_mode_to_opengl_type(WrapMode mode) {
		switch (mode) {
		case WrapMode::Repeat: return GL_REPEAT;
		case WrapMode::MirroredRepeat: return GL_MIRRORED_REPEAT;
		case WrapMode::ClampToEdge: return GL_CLAMP_TO_EDGE;
		case WrapMode::ClampToBorder: return GL_CLAMP_TO_BORDER;
		}
		CORE_ASSERT(false, "unknown wrap mode!");
		return 0;
	}

	static size_t image_format_to_opengl_data_size(ImageFormat format) {
		switch (format) {
		case ImageFormat::RGBA8: return 4;
		case ImageFormat::RGBA32F: return 16;
		case ImageFormat::Depth: return 3;
		case ImageFormat::DepthStencil: return 4;
		}
		CORE_ASSERT(false, "unknown image format!");
		return 0;
	}

	// OpenGLImage2D definitions
	// --------------------------------------------------

	OpenGLImage2D::OpenGLImage2D(void const* data, size_t size, Description const& desc) {
		m_description = desc;
		CORE_ASSERT(desc.m_depth == 1, "Image2D must have only 1 layer!");

		glGenTextures(1, &m_handle);
		glBindTexture(GL_TEXTURE_2D, m_handle);

		CORE_ASSERT((size == 0) | (size == m_description.m_width * m_description.m_height * image_format_to_opengl_data_size(m_description.m_format)),
			"data size must match the whole image!");

		glTexImage2D(
			GL_TEXTURE_2D, 0,
			image_format_to_opengl_internal_format(desc.m_format),
			desc.m_width, desc.m_height, 0,
			image_format_to_opengl_format(desc.m_format),
			image_format_to_opengl_data_type(desc.m_format),
			data);

		if (desc.m_mipmap) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler_mode_to_opengl_mipmap_type(desc.m_sampler_mode));
		}
		else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler_mode_to_opengl_type(desc.m_sampler_mode));
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler_mode_to_opengl_type(desc.m_sampler_mode));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode_to_opengl_type(desc.m_wrap_mode));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode_to_opengl_type(desc.m_wrap_mode));

		if (desc.m_mipmap) {
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	OpenGLImage2D::~OpenGLImage2D() {
		if (m_handle == 0) return;
		glDeleteTextures(1, &m_handle);
	}

	void OpenGLImage2D::bind(uint32_t binding) const {
		glActiveTexture(GL_TEXTURE0 + binding);
		glBindTexture(GL_TEXTURE_2D, m_handle);
	}

	void OpenGLImage2D::unbind(uint32_t binding) const {
		glActiveTexture(GL_TEXTURE0 + binding);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void OpenGLImage2D::write(void const* data, size_t size) const {
		PROFILE_FUNCTION();
		size_t bytePerPixel = image_format_to_opengl_data_size(m_description.m_format);
		CORE_ASSERT(size == m_description.m_width * m_description.m_height * bytePerPixel, "data size must match the whole image!");
		glTextureSubImage2D(
			m_handle, 0, 0, 0,
			m_description.m_width, m_description.m_height,
			image_format_to_opengl_format(m_description.m_format),
			image_format_to_opengl_data_type(m_description.m_format),
			data);
	}

	static void* GetDataFromFaces(GLenum target, ImageCube::Faces const& data) {
		switch (target) {
		case GL_TEXTURE_CUBE_MAP_POSITIVE_X: return data.m_right;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_X: return data.m_left;
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Y: return data.m_top;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y: return data.m_bottom;
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Z: return data.m_front;
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: return data.m_back;
		}
		CORE_ASSERT(false, "unknown face!");
		return nullptr;
	}

	OpenGLImageCube::OpenGLImageCube(Faces data, size_t size, Description const& desc) {
		m_description = desc;
		CORE_ASSERT(desc.m_depth == 6, "ImageCube must have exactly 6 layers!");

		glGenTextures(1, &m_handle);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_handle);

		CORE_ASSERT((size == 0) | (size == m_description.m_width * m_description.m_height * 6 * image_format_to_opengl_data_size(m_description.m_format)),
			"data size must match the whole image!");

		for (int i = 0; i < 6; ++i) {
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
				image_format_to_opengl_internal_format(desc.m_format),
				desc.m_width, desc.m_height, 0,
				image_format_to_opengl_format(desc.m_format),
				image_format_to_opengl_data_type(desc.m_format),
				GetDataFromFaces(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, data));
		}

		if (desc.m_mipmap) {
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, sampler_mode_to_opengl_mipmap_type(desc.m_sampler_mode));
		}
		else {
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, sampler_mode_to_opengl_type(desc.m_sampler_mode));
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, sampler_mode_to_opengl_type(desc.m_sampler_mode));
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap_mode_to_opengl_type(desc.m_wrap_mode));
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap_mode_to_opengl_type(desc.m_wrap_mode));
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrap_mode_to_opengl_type(desc.m_wrap_mode));

		if (desc.m_mipmap) {
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	OpenGLImageCube::~OpenGLImageCube() {
		if (m_handle == 0) return;
		glDeleteTextures(1, &m_handle);
	}

	void OpenGLImageCube::bind(uint32_t binding) const {
		glActiveTexture(GL_TEXTURE0 + binding);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_handle);
	}

	void OpenGLImageCube::unbind(uint32_t binding) const {
		glActiveTexture(GL_TEXTURE0 + binding);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	void OpenGLImageCube::write(void const* data, size_t size) const {
		PROFILE_FUNCTION();
		size_t bytePerPixel = image_format_to_opengl_data_size(m_description.m_format);
		CORE_ASSERT(size == m_description.m_width * m_description.m_height * 6 * bytePerPixel, "data size must match the whole image!");
		glTextureSubImage2D(
			m_handle, 0, 0, 0,
			m_description.m_width, m_description.m_height,
			image_format_to_opengl_format(m_description.m_format),
			image_format_to_opengl_data_type(m_description.m_format),
			data);
	}

}
