#pragma once

#include "core/core.h"
#include "render/framebuffer.h"
#include "render/rhi/opengl_image.h"

namespace z1 {

	struct OpenGLFramebuffer : Framebuffer {
		OpenGLFramebuffer(
			uint32_t width, uint32_t height,
			std::initializer_list<Attachment> attachments);
		~OpenGLFramebuffer() override;

		void bind() const override;
		void unbind() const override;

		void resize(uint32_t width, uint32_t height) override;
		void read_pixel(uint32_t attachment, uint32_t x, uint32_t y, void* data) const override;
		void read_pixels(uint32_t attachment, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void* data) const override;

		void bind_attachment(uint32_t index, uint32_t binding) const override;

		void* get_native_handle() const override { return (void*)(uint64_t)m_handle; }
		void* get_attachment_native_handle(uint32_t attachment) const override;

	private:
		void create();
		void destroy();

		uint32_t m_handle = 0;
		std::vector<Attachment> m_attachments;
		std::vector<GLenum> m_attachment_ids;
		std::vector<OpenGLImage2D*> m_attachment_images;
		uint32_t m_depth_stencil_attachment_index = INVALID_INDEX;
	};

}
