#pragma once

#include "core/core.h"
#include "render/render_pass.h"
#include "render/image.h"
#include "render/framebuffer.h"

namespace z1 {

	struct RenderGraph;

	struct API RenderGraphNode {

		RenderGraphNode() = default;
		~RenderGraphNode() {}

		RenderGraphNode& add_input(std::string const& name);
		RenderGraphNode& set_resolution(uint32_t width, uint32_t height);
		RenderGraphNode& set_resolution_as(std::shared_ptr<Framebuffer> const& framebuffer);
		RenderGraphNode& add_output(
			std::string const& name,
			ImageFormat format,
			SamplerMode sampler_mode = SamplerMode::Linear,
			WrapMode wrap_mode = WrapMode::Repeat);
		RenderGraphNode& set_output(std::shared_ptr<Framebuffer> const& framebuffer);
		RenderGraphNode& set_passthrough(std::string const& pass_name);
		RenderGraphNode& set_pass_desc(RenderPass::Description const& desc);
		RenderGraphNode& depends_on(std::string const& pass_name);

		RenderGraphNode& pre_pass(std::function<void(RenderGraphNode&, GraphicsContext&)> const& func);
		RenderGraphNode& execute(std::function<void(RenderGraphNode&, GraphicsContext&)> const& func);
		RenderGraphNode& post_pass(std::function<void(RenderGraphNode&, GraphicsContext&)> const& func);

		uint32_t bind_input_index(uint32_t index);
		uint32_t bind_input_name(std::string const& name);
		std::shared_ptr<Image> get_input_image_index(uint32_t index);
		std::shared_ptr<Image> get_input_image_name(std::string const& name);
		void unbind_input_index(uint32_t index);
		void unbind_input_name(std::string const& name);

		float get_aspect() const {
			return (float)m_width / (float)m_height;
		}

		std::shared_ptr<Framebuffer> const& get_output() const {
			return m_output;
		}

	private:
		friend struct RenderGraph;

		std::string m_name;
		std::vector<std::string> m_inputs;
		std::vector<std::string> m_manual_depends;
		std::vector<std::pair<std::string, Framebuffer::Attachment>> m_output_spec;
		std::shared_ptr<Framebuffer> m_output;
		std::string m_passthrough_pass;
		RenderPass::Description m_pass_desc;

		uint32_t m_width = 0;
		uint32_t m_height = 0;

		// filled after compile
		std::unordered_map<std::string, uint32_t> m_output_to_attachment_id;
		std::unordered_set<int> m_depends;
		std::unordered_map<std::string, std::pair<RenderGraphNode*, int>> m_inputs_by_name;
		std::unordered_map<int, std::pair<RenderGraphNode*, int>> m_inputs_by_index;
		std::shared_ptr<RenderPass> m_render_pass;

		std::function<void(RenderGraphNode&, GraphicsContext&)> m_pre_pass_func;
		std::function<void(RenderGraphNode&, GraphicsContext&)> m_exec_func;
		std::function<void(RenderGraphNode&, GraphicsContext&)> m_post_pass_func;
	};

	struct API RenderGraph {

		RenderGraphNode& add_pass(std::string const& name);

		void compile();
		void execute();

		static void clear_cache();

	private:

		bool topo_sort(std::vector<int>& order);
		bool cache_is_reusable(std::shared_ptr<Framebuffer> const& cache, RenderGraphNode const& node);

		bool m_compiled = false;

		// before compile
		std::vector<RenderGraphNode> m_nodes;

		// after compile
		std::vector<int> m_exec_order;

		// framebuffer pool
		static std::unordered_map<std::string, std::shared_ptr<Framebuffer>> s_cached_framebuffers;

	};

}
