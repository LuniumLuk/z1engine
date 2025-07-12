#pragma once

#include "render/image.h"

namespace z1 {

	struct API Framebuffer {

		struct Attachment {
			ImageFormat m_format;
			SamplerMode m_sampler_mode;
			WrapMode m_wrap_mode;
		};

		struct Description {
			uint32_t m_width;
			uint32_t m_height;
		};

		virtual ~Framebuffer() = default;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual void resize(uint32_t width, uint32_t height) = 0;
		virtual void read_pixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height, void const* data) const = 0;

		virtual void bind_attachment(uint32_t index, uint32_t binding) const = 0;

		virtual void* get_native_handle() const = 0;
		virtual void* get_attachment_native_handle(uint32_t attachment) const = 0;

		Description const& get_description() const { return m_description; }

		static std::shared_ptr<Framebuffer> create(
			uint32_t width, uint32_t height,
			std::initializer_list<Attachment> attachments);

	protected:
		Description m_description;
	};

}
