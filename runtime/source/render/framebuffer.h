#pragma once

#include "core/core.h"
#include "core/window.h"
#include "render/resource.h"
#include "render/image.h"

namespace z1 {

	struct API Framebuffer : RenderResource {

		struct Attachment {
			ImageFormat format;
			SamplerMode sampler_mode;
			WrapMode wrap_mode;
		};

		struct Description {
			uint32_t width = 0;
			uint32_t height = 0;
		};

		Framebuffer() : RenderResource(ResourceType::Framebuffer) {}

		virtual ~Framebuffer() = default;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual void resize(uint32_t width, uint32_t height) = 0;
		virtual void read_pixel(uint32_t attachment, uint32_t x, uint32_t y, void* data) const = 0;
		virtual void read_pixels(uint32_t attachment, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void* data) const = 0;

		virtual void bind_attachment(uint32_t index, uint32_t binding) const = 0;

		virtual void* get_native_handle() const = 0;
		virtual void* get_attachment_native_handle(uint32_t attachment) const = 0;

		virtual std::shared_ptr<Image> get_attachment_image(uint32_t attachment) const = 0;

		Description const& get_description() const { return m_description; }

		static std::shared_ptr<Framebuffer> create(
			uint32_t width, uint32_t height,
			std::initializer_list<Attachment> attachments);

		static std::shared_ptr<Framebuffer> create(
			uint32_t width, uint32_t height,
			std::vector<Attachment> attachments);

		virtual uint32_t get_width() const { return m_description.width; }
		virtual uint32_t get_height() const { return m_description.height; }

		std::vector<Attachment> const& get_attachments() const { return m_attachments; }

	protected:
		Description m_description = {};
		std::vector<Attachment> m_attachments;
	};

}
