#pragma once

#include "core/core.h"
#include <deque>
#include <vector>
#include <numeric>
#include <algorithm>

namespace z1 {

	struct Framebuffer;
	struct Pipeline;
	struct RenderPass;

	struct RenderStats {
		uint32_t draw_calls = 0;
		uint32_t visible_objects = 0;
		uint32_t culled_objects = 0;

		float fps = 0.0f;
		float low_5_percent = 0.0f;
		float low_1_percent = 0.0f;
		float frame_time = 0.0f;

		std::vector<std::pair<std::string, uint32_t>> counters;
		uint32_t* current_counter = nullptr;

		void reset() {
			draw_calls = 0;
			visible_objects = 0;
			culled_objects = 0;
			counters.clear();
			current_counter = nullptr;
		}

		void begin_counter(std::string const& name) {
			counters.push_back({ name, 0 });
			current_counter = &counters.back().second;
		}

		void end_counter() {
			current_counter = nullptr;
		}

		void increment_counter(uint32_t value = 1) {
			if (current_counter)
				*current_counter += value;
			draw_calls += value;
		}

	};

	struct API GraphicsContext {
		virtual void init() = 0;
		virtual void begin_frame() = 0;
		virtual void end_frame() = 0;
		virtual void swap_buffers() = 0;
		virtual void finish() = 0;

		static std::shared_ptr<GraphicsContext> create();

		void update_stats(float dt);

		virtual void bind_framebuffer(std::shared_ptr<Framebuffer> const& framebuffer) = 0;
		virtual void bind_pipeline(std::shared_ptr<Pipeline> const& pipeline) = 0;

		virtual void exec_render_pass(std::shared_ptr<RenderPass> const& render_pass) = 0;

		virtual void set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void set_scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

		virtual void push_debug_group(std::string const& name) {}
		virtual void pop_debug_group() {}

		uint32_t m_max_image_binding_count = 0;
		uint32_t m_max_uniform_buffer_binding_count = 0;

		uint32_t acquire_image_binding();
		void release_image_binding(uint32_t binding);

		uint32_t acquire_uniform_buffer_binding();
		void release_uniform_buffer_binding(uint32_t binding);

		virtual void blit_attachment(
			std::shared_ptr<Framebuffer> const& src,
			std::shared_ptr<Framebuffer> const& dst,
			uint32_t src_attachment = 0,
			uint32_t dst_attachment = 0,
			uint32_t src_x = 0, uint32_t src_y = 0,
			uint32_t dst_x = 0, uint32_t dst_y = 0,
			uint32_t width = NUM_MAX, uint32_t height = NUM_MAX) = 0;

		virtual void blit_depth_stencil(
			std::shared_ptr<Framebuffer> const& src,
			std::shared_ptr<Framebuffer> const& dst,
			uint32_t src_x = 0, uint32_t src_y = 0,
			uint32_t dst_x = 0, uint32_t dst_y = 0,
			uint32_t width = NUM_MAX, uint32_t height = NUM_MAX) = 0;

		std::shared_ptr<Framebuffer> m_swapchain_framebuffer = nullptr;
		std::shared_ptr<Framebuffer> m_current_framebuffer = nullptr;
		std::shared_ptr<Pipeline> m_current_pipeline = nullptr;

		RenderStats m_stats;

	private:
		std::deque<float> m_frame_time_history;

	protected:
		std::stack<uint32_t> m_free_image_bindings;
		std::stack<uint32_t> m_free_uniform_buffer_bindings;
	};

}
