#pragma once

#include "render/graphics_context.h"

struct GLFWwindow;

namespace z1 {

	void glCheckError_(const char *file, int line);

	// Macro to make calling it easier
	#define glCheckError() glCheckError_(__FILE__, __LINE__)

	struct OpenGLContext : GraphicsContext {
		OpenGLContext();

		void init() override;
		void begin_frame() override; // opens the frame's gpu timer query when probing is enabled
		void end_frame() override {}
		void swap_buffers() override;
		void finish() override {}

		void bind_framebuffer(std::shared_ptr<Framebuffer> const& framebuffer) override;
		void bind_pipeline(std::shared_ptr<Pipeline> const& pipeline) override;

		void exec_render_pass(std::shared_ptr<RenderPass> const& render_pass) override;

		void set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		void set_scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		void push_debug_group(std::string const& name) override;
		void pop_debug_group() override;

		void blit_attachment(
			std::shared_ptr<Framebuffer> const& src,
			std::shared_ptr<Framebuffer> const& dst,
			uint32_t src_attachment,
			uint32_t dst_attachment,
			uint32_t src_x, uint32_t src_y,
			uint32_t dst_x, uint32_t dst_y,
			uint32_t width, uint32_t height) override;

		void blit_depth_stencil(
			std::shared_ptr<Framebuffer> const& src,
			std::shared_ptr<Framebuffer> const& dst,
			uint32_t src_x, uint32_t src_y,
			uint32_t dst_x, uint32_t dst_y,
			uint32_t width, uint32_t height) override;

		void bind_texture_unit(uint32_t unit, uint32_t gl_handle, TextureTarget target) override;
		void bind_uniform_buffer(uint32_t binding, UniformBuffer const& buffer) override;
		void notify_texture_bound(uint32_t unit, uint32_t gl_handle, TextureTarget target) override;
		void notify_uniform_buffer_bound(uint32_t binding, uint32_t gl_handle) override;
		uint32_t get_fallback_texture(TextureTarget target) const override;

	private:
		void create_default_sampler_textures();

		GLFWwindow*  m_window;
		uint32_t m_debug_group_depth = 0; // balance guard for push/pop debug groups
		uint32_t m_default_sampler_texture_2d = 0;       // 1x1 white texture
		uint32_t m_default_sampler_texture_2d_array = 0; // 1x1x1 white texture
		uint32_t m_default_sampler_texture_cube = 0;     // 1x1x6 white texture

		// Binder dedup caches (see GraphicsContext::bind_texture_unit/bind_uniform_buffer).
		std::vector<uint32_t> m_bound_texture_handles;
		std::vector<uint8_t>  m_bound_texture_targets;
		std::vector<uint32_t> m_bound_uniform_buffer_handles;
	};

}
