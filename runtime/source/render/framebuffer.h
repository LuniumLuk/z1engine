#pragma once

#include "core/core.h"
#include "core/window.h"
#include "render/image.h"

namespace z1 {

	struct API Framebuffer {

		struct Attachment {
			ImageFormat format;
			SamplerMode sampler_mode;
			WrapMode wrap_mode;
		};

		struct Description {
			uint32_t width;
			uint32_t height;
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

		static inline uint32_t get_width(std::shared_ptr<Framebuffer> const& framebuffer) noexcept {
			return framebuffer ? framebuffer->get_description().width : g_runtime_context.m_window->get_width();
		}

		static inline uint32_t get_height(std::shared_ptr<Framebuffer> const& framebuffer) noexcept {
			return framebuffer ? framebuffer->get_description().height : g_runtime_context.m_window->get_height();
		}

	protected:
		Description m_description;
	};

}
