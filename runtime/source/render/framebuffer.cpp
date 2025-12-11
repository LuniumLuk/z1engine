#include "pch.h"
#include "render/framebuffer.h"
#include "render/rhi/opengl_framebuffer.h"

namespace z1 {

	std::shared_ptr<Framebuffer> Framebuffer::create(
		uint32_t width, uint32_t height,
		std::initializer_list<Attachment> attachments) {
		PROFILE_FUNCTION();

		return std::shared_ptr<Framebuffer>(new OpenGLFramebuffer(width, height, attachments));
	}

	std::shared_ptr<Framebuffer> Framebuffer::create(
		uint32_t width, uint32_t height,
		std::vector<Attachment> attachments) {
		PROFILE_FUNCTION();

		return std::shared_ptr<Framebuffer>(new OpenGLFramebuffer(width, height, attachments));
	}

}
