#include "pch.h"
#include "render/rhi/opengl_framebuffer.h"
#include "glad/glad.h"

namespace z1 {

    static GLenum image_format_to_attachment_type(ImageFormat format, uint32_t binding) {
        switch (format) {
        case ImageFormat::RGBA8:
        case ImageFormat::RGBA32F:
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
        std::initializer_list<Attachment> attachments) {

        m_attachments = attachments;

        m_description.m_width = width;
        m_description.m_height = height;

        create();
    }

    OpenGLFramebuffer::~OpenGLFramebuffer() {
        destroy();
    }

    void OpenGLFramebuffer::create() {
        glGenFramebuffers(1, &m_handle);
        glBindFramebuffer(GL_FRAMEBUFFER, m_handle);

        uint32_t binding = 0;

        for (auto const& attachment : m_attachments) {
            Image::Description desc{};
            desc.m_width = m_description.m_width;
            desc.m_height = m_description.m_height;
            desc.m_depth = 1;
            desc.m_format = attachment.m_format;
            desc.m_sampler_mode = attachment.m_sampler_mode;
            desc.m_wrap_mode = attachment.m_wrap_mode;
            desc.m_mipmap = false;

            auto image = new OpenGLImage2D(nullptr, 0, desc);

            glBindTexture(GL_TEXTURE_2D, image->m_handle);
            glFramebufferTexture(GL_FRAMEBUFFER, image_format_to_attachment_type(attachment.m_format, binding), image->m_handle, 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            switch (attachment.m_format) {
            case ImageFormat::RGBA8:
            case ImageFormat::RGBA32F:
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
        for (auto image : m_attachment_images) {
            delete image;
        }
        m_attachment_images.clear();
        m_depth_stencil_attachment_index = INVALID_INDEX;
    }

    void OpenGLFramebuffer::bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
    }

    void OpenGLFramebuffer::unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::resize(uint32_t width, uint32_t height) {
        PROFILE_FUNCTION();
        destroy();
        m_description.m_width = width;
        m_description.m_height = height;
        create();
    }

    void OpenGLFramebuffer::read_pixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height, void* data) const {
        PROFILE_FUNCTION();
        UNIMPLEMENTED_FUNCTION();
    }

    void OpenGLFramebuffer::bind_attachment(uint32_t index, uint32_t binding) const {
        if (index > m_attachment_images.size()) {
            CORE_ASSERT(false, "index out of range!");
            return;
        }

        glActiveTexture(GL_TEXTURE0 + binding);
        glBindTexture(GL_TEXTURE_2D, m_attachment_images[index]->m_handle);
    }

    void* OpenGLFramebuffer::get_attachment_native_handle(uint32_t attachment) const {
        if (attachment < m_attachment_images.size()) {
            return m_attachment_images[attachment]->get_native_handle();
        }
        CORE_WARN("attachment required is out of range!");
        return nullptr;
    }


}
