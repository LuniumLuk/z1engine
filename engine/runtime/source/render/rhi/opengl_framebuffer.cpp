#include "pch.h"
#include "render/rhi/opengl_framebuffer.h"
#include "glad/glad.h"

namespace z1 {

	static GLenum image_format_to_attachment_type(ImageFormat format, uint32_t binding) {
		switch (format) {
		case ImageFormat::RGBA8:
		case ImageFormat::RGBA32F:
		case ImageFormat::RGB16F:
		case ImageFormat::RG16F:
			return GL_COLOR_ATTACHMENT0 + binding;
		case ImageFormat::Depth:
			return GL_DEPTH_ATTACHMENT;
		case ImageFormat::DepthStencil:
			return GL_DEPTH_STENCIL_ATTACHMENT;
		}
		CORE_ASSERT(false, "unknown image format!");
		return 0;
	}

	OpenGLFramebuffer::OpenGLFramebuffer(
		uint32_t width, uint32_t height,
		std::vector<Attachment> attachments) {

		m_attachments = attachments;

		m_description.width = width;
		m_description.height = height;

		create();
	}

	OpenGLFramebuffer::~OpenGLFramebuffer() {
		destroy();
	}

	void OpenGLFramebuffer::create() {
		glGenFramebuffers(1, &m_handle);
		glBindFramebuffer(GL_FRAMEBUFFER, m_handle);

		uint32_t binding = 0;

		m_attachment_ids.clear();
		for (auto const& attachment : m_attachments) {
			std::shared_ptr<Image> image;
			if (attachment.layers > 1) {
				image = Image2DArray::create(
					nullptr, 0,
					m_description.width,
					m_description.height,
					attachment.layers,
					attachment.format,
					attachment.sampler_mode,
					attachment.wrap_mode,
					false);
			}
			else {
				image = Image2D::create(
					nullptr, 0,
					m_description.width,
					m_description.height,
					attachment.format,
					attachment.sampler_mode,
					attachment.wrap_mode,
					false);
			}
			auto attachment_id = image_format_to_attachment_type(attachment.format, binding);

			GLuint native_handle = (GLuint)reinterpret_cast<uintptr_t>(image->get_native_handle());

			if (attachment.layers > 1) {
				glBindTexture(GL_TEXTURE_2D_ARRAY, native_handle);
				glFramebufferTexture(GL_FRAMEBUFFER, attachment_id, native_handle, 0);
				glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
			}
			else {
				glBindTexture(GL_TEXTURE_2D, native_handle);
				glFramebufferTexture(GL_FRAMEBUFFER, attachment_id, native_handle, 0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			m_attachment_ids.push_back(attachment_id);

			switch (attachment.format) {
			case ImageFormat::RGBA8:
			case ImageFormat::RGBA32F:
			case ImageFormat::RGB16F:
			case ImageFormat::RG16F:
				binding++;
				break;
			case ImageFormat::Depth:
			case ImageFormat::DepthStencil:
				if (m_depth_stencil_attachment_index != INVALID_INDEX) {
					CORE_ASSERT(false, "only one depth/stencil attachment is allowed!");
					return;
				}
				m_depth_stencil_attachment_index = (uint32_t)m_attachment_images.size();
				break;
			}

			m_attachment_images.push_back(image);
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			CORE_ASSERT(false, "framebuffer is not complete!");
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::destroy() {
		if (m_handle == 0) return;
		glDeleteFramebuffers(1, &m_handle);
		m_attachment_images.clear();
		m_depth_stencil_attachment_index = INVALID_INDEX;
	}

	void OpenGLFramebuffer::bind() const {
		glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
		std::vector<GLuint> color_attachments;
		for (uint32_t i = 0; i < m_attachment_ids.size(); ++i) {
			if (i == m_depth_stencil_attachment_index)
				continue;

			color_attachments.push_back(m_attachment_ids[i]);
		}
		glDrawBuffers((GLsizei)color_attachments.size(), color_attachments.data());
	}

	void OpenGLFramebuffer::unbind() const {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::resize(uint32_t width, uint32_t height) {
		PROFILE_FUNCTION();
		destroy();
		m_description.width = width;
		m_description.height = height;
		create();
	}

	void OpenGLFramebuffer::read_pixel(uint32_t attachment, uint32_t x, uint32_t y, void* data) const {
		read_pixels(attachment, x, y, 1, 1, data);
	}

	void OpenGLFramebuffer::read_pixels(uint32_t attachment, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void* data) const {
		PROFILE_FUNCTION();

		if (attachment >= m_attachments.size()) {
			CORE_WARN("attachment required is out of range!");
			return;
		}

		bind();
		glReadBuffer(m_attachment_ids[attachment]);
		glReadPixels(
			x, y, width, height,
			image_format_to_opengl_format(m_attachments[attachment].format),
			image_format_to_opengl_data_type(m_attachments[attachment].format),
			data);
		unbind();
	}

	void OpenGLFramebuffer::bind_attachment(uint32_t index, uint32_t binding) const {
		if (index > m_attachment_images.size()) {
			CORE_ASSERT(false, "index out of range!");
			return;
		}

		GLuint native_handle = (GLuint)reinterpret_cast<uintptr_t>(m_attachment_images[index]->get_native_handle());
		glActiveTexture(GL_TEXTURE0 + binding);

		if (m_attachments[index].layers > 1) {
			glBindTexture(GL_TEXTURE_2D_ARRAY, native_handle);
		}
		else {
			glBindTexture(GL_TEXTURE_2D, native_handle);
		}
	}

	void OpenGLFramebuffer::set_attachment_layer(uint32_t index, int layer) {
		if (index >= m_attachment_images.size()) return;
		auto id = m_attachment_ids[index];
		auto image = m_attachment_images[index];
		GLuint texture = (GLuint)reinterpret_cast<uintptr_t>(image->get_native_handle());

		//bind();
		if (layer < 0) {
			// Attach whole texture (if layered, it attaches all layers)
			glFramebufferTexture(GL_FRAMEBUFFER, id, texture, 0);
		}
		else {
			glFramebufferTextureLayer(GL_FRAMEBUFFER, id, texture, 0, layer);
		}
		//unbind();
	}

	std::shared_ptr<Image> OpenGLFramebuffer::get_attachment_image(uint32_t attachment) const {
		if (attachment < m_attachment_images.size()) {
			return m_attachment_images[attachment];
		}
		CORE_WARN("attachment required is out of range!");
		return nullptr;
	}

	void* OpenGLFramebuffer::get_attachment_native_handle(uint32_t attachment) const {
		if (attachment < m_attachment_images.size()) {
			return m_attachment_images[attachment]->get_native_handle();
		}
		CORE_WARN("attachment required is out of range!");
		return nullptr;
	}

	void OpenGLSwapChainFramebuffer::bind() const {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	uint32_t OpenGLSwapChainFramebuffer::get_width() const {
		return g_runtime_context.m_window->get_width();
	}

	uint32_t OpenGLSwapChainFramebuffer::get_height() const {
		return g_runtime_context.m_window->get_height();
	}

}
