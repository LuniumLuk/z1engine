#pragma once

#include "render/image.h"

namespace z1 {

	struct OpenGLImage2D : Image2D {
		friend struct OpenGLFramebuffer;

		OpenGLImage2D(void* data, size_t size, Description const& desc);
		~OpenGLImage2D() override;

		void bind(uint32_t binding) const override;
		void unbind(uint32_t binding) const override;
		void write(void* data, size_t size) const override;

		void* get_native_handle() const override { return (void*)(uint64_t)m_handle; }

	private:
		uint32_t m_handle = 0;
	};

	struct OpenGLImageCube : ImageCube {
		OpenGLImageCube(Faces data, size_t size, Description const& desc);
		~OpenGLImageCube() override;

		void bind(uint32_t binding) const override;
		void unbind(uint32_t binding) const override;
		void write(void* data, size_t size) const override;

		void* get_native_handle() const override { return (void*)(uint64_t)m_handle; }

	private:
		uint32_t m_handle = 0;
	};

}
