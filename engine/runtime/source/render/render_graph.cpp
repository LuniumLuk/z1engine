#include "pch.h"

#include "render/render_graph.h"
#include "render/graphics_context.h"

namespace z1 {

	std::unordered_map<std::string, std::shared_ptr<Framebuffer>> RenderGraph::s_cached_framebuffers;

	RenderGraphNode& RenderGraphNode::add_input(std::string const& name) {
		m_inputs.push_back(name);
		return *this;
	}

	RenderGraphNode& RenderGraphNode::set_resolution(uint32_t width, uint32_t height) {
		m_width = width;
		m_height = height;
		return *this;
	}

	RenderGraphNode& RenderGraphNode::set_resolution_as(std::shared_ptr<Framebuffer> const& framebuffer) {
		m_width = framebuffer->get_width();
		m_height = framebuffer->get_height();
		return *this;
	}

	RenderGraphNode& RenderGraphNode::add_output(
		std::string const& name,
		ImageFormat format,
		SamplerMode sampler_mode /*= SamplerMode::Linear*/,
		WrapMode wrap_mode /*= WrapMode::Repeat*/) {

		Framebuffer::Attachment attach{};
		attach.format = format;
		attach.sampler_mode = sampler_mode;
		attach.wrap_mode = wrap_mode;
		m_output_spec[name] = attach;
		return *this;
	}

	RenderGraphNode& RenderGraphNode::set_output(std::shared_ptr<Framebuffer> const& framebuffer) {
		m_output = framebuffer;
		return *this;
	}

	RenderGraphNode& RenderGraphNode::set_passthrough(std::string const& pass_name) {
		m_manual_depends.push_back(pass_name);
		m_passthrough_pass = pass_name;
		return *this;
	}

	RenderGraphNode& RenderGraphNode::set_pass_desc(RenderPass::Description const& desc) {
		m_pass_desc = desc;
		return *this;
	}

	RenderGraphNode& RenderGraphNode::depends_on(std::string const& pass_name) {
		m_manual_depends.push_back(pass_name);
		return *this;
	}

	RenderGraphNode& RenderGraphNode::pre_pass(std::function<void(RenderGraphNode&, GraphicsContext&)> const& func) {
		m_pre_pass_func = func;
		return *this;
	}

	RenderGraphNode& RenderGraphNode::execute(std::function<void(RenderGraphNode&, GraphicsContext&)> const& func) {
		m_exec_func = func;
		return *this;
	}

	RenderGraphNode& RenderGraphNode::post_pass(std::function<void(RenderGraphNode&, GraphicsContext&)> const& func) {
		m_post_pass_func = func;
		return *this;
	}

	uint32_t RenderGraphNode::bind_input_index(uint32_t index) {
		auto image = get_input_image_index(index);
		image->bind();
		return image->get_binding();
	}

	uint32_t RenderGraphNode::bind_input_name(std::string const& name) {
		auto image = get_input_image_name(name);
		image->bind();
		return image->get_binding();
	}

	std::shared_ptr<Image> RenderGraphNode::get_input_image_index(uint32_t index) {
		auto [node, attachment_id] = m_inputs_by_index[index];
		return node->m_output->get_attachment_image(attachment_id);
	}

	std::shared_ptr<Image> RenderGraphNode::get_input_image_name(std::string const& name) {
		auto [node, attachment_id] = m_inputs_by_name[name];
		return node->m_output->get_attachment_image(attachment_id);
	}

	void RenderGraphNode::unbind_input_index(uint32_t index) {
		auto image = get_input_image_index(index);
		image->unbind();
	}

	void RenderGraphNode::unbind_input_name(std::string const& name) {
		auto image = get_input_image_name(name);
		image->unbind();
	}


	RenderGraphNode& RenderGraph::add_pass(std::string const& name) {
		m_nodes.emplace_back();
		m_nodes.back().m_name = name;
		return m_nodes.back();
	}

	bool RenderGraph::topo_sort(std::vector<int>& order) {
		const int N = static_cast<int>(m_nodes.size());
		order.clear();
		order.reserve(N);

		// Compute in-degrees
		std::vector<int> indeg(N, 0);
		for (int i = 0; i < N; i++) {
			for (int d : m_nodes[i].m_depends) {
				if (d < 0 || d >= N) return false; // invalid index
				indeg[i]++;    // node i depends on d -> d -> i
			}
		}

		// Collect nodes with in-degree 0
		std::queue<int> q;
		for (int i = 0; i < N; i++) {
			if (indeg[i] == 0)
				q.push(i);
		}

		// Kahn's algorithm
		while (!q.empty()) {
			int u = q.front();
			q.pop();
			order.push_back(u);

			// Removing node u, so every node that depends on u loses 1 indegree
			for (int v = 0; v < N; v++) {
				// Check if v depends on u
				for (int d : m_nodes[v].m_depends) {
					if (d == u) {
						if (--indeg[v] == 0) {
							q.push(v);
						}
					}
				}
			}
		}

		// If not all nodes were processed, there's a cycle
		return (order.size() == N);
	}

	bool RenderGraph::cache_is_reusable(std::shared_ptr<Framebuffer> const& cache, RenderGraphNode const& node) {
		if (cache->get_attachments().size() != node.m_output_spec.size()) return false;

		int attachment_id = 0;
		for (auto& [_, spec] : node.m_output_spec) {
			auto const& cached_spec = cache->get_attachments()[attachment_id];

			if (cached_spec.format != spec.format) return false;
			if (cached_spec.sampler_mode != spec.sampler_mode) return false;
			if (cached_spec.wrap_mode != spec.wrap_mode) return false;

			attachment_id += 1;
		}

		return true;
	}

	void RenderGraph::compile() {
		if (m_compiled) {
			CORE_WARN("RenderGraph: already compiled");
			return;
		}

		// pass #1: build DAG graph of nodes
		int idx = 0;
		std::unordered_map<std::string, int> name_to_id;
		std::unordered_map<std::string, std::vector<int>> output_to_ids;
		for (auto& node : m_nodes) {
			if (name_to_id.find(node.m_name) != name_to_id.end()) {
				DEBUG_CHECK(false);
				CORE_ERROR("RenderGraph: duplicate node name: {} found", node.m_name);
				return;
			}
			name_to_id[node.m_name] = idx;

			// parse outputs
			uint32_t attachment_idx = 0;
			for (auto& [output, spec] : node.m_output_spec) {
				if (output_to_ids.find(output) != output_to_ids.end()) {
					DEBUG_CHECK(false);
					CORE_ERROR("RenderGraph: duplicate output: {} found, node: {}", output, node.m_name);
					return;
				}
				output_to_ids[output] = { idx };
				node.m_output_to_attachment_id[output] = attachment_idx;
				attachment_idx += 1;
			}
			idx += 1;
		}

		// add passthrough nodes to output dependencies
		for (auto& node : m_nodes) {
			if (!node.m_passthrough_pass.empty()) {
				auto const& src_pass = m_nodes[name_to_id[node.m_passthrough_pass]];
				for (auto& [output, _] : src_pass.m_output_spec) {
					output_to_ids[output].push_back(name_to_id[node.m_name]);
				}
			}
		}

		for (auto& node : m_nodes) {
			// parse inputs
			int input_idx = 0;
			for (auto& input : node.m_inputs) {
				if (output_to_ids.find(input) == output_to_ids.end()) {
					DEBUG_CHECK(false);
					CORE_ERROR("RenderGraph: input: {} not found, node: {}", input, node.m_name);
					return;
				}
				for (int src_node_id : output_to_ids[input]) {
					node.m_depends.insert(src_node_id);
				}

				int src_node_id = output_to_ids[input][0];
				int attachment_id = m_nodes[src_node_id].m_output_to_attachment_id[input];
				node.m_inputs_by_index[input_idx] = std::make_pair(&m_nodes[src_node_id], attachment_id);
				node.m_inputs_by_name[input] = std::make_pair(&m_nodes[src_node_id], attachment_id);

				input_idx += 1;
			}

			// manual dependencies
			for (auto& dep : node.m_manual_depends) {
				if (name_to_id.find(dep) == name_to_id.end()) {
					DEBUG_CHECK(false);
					CORE_ERROR("RenderGraph: dependency: {} not found, node: {}", dep, node.m_name);
					return;
				}
				node.m_depends.insert(name_to_id[dep]);
			}
		}

		// pass #2: sort the nodes
		m_exec_order.clear();
		m_exec_order.reserve(m_nodes.size());
		topo_sort(m_exec_order);

		// pass #3: build or reuse framebuffers
		for (auto& node : m_nodes) {
			if (node.m_output)
				continue;

			if (!node.m_passthrough_pass.empty()) {
				// passthrought: inherit output from another node
				continue;
			}

			if (node.m_width == 0 || node.m_height == 0) {
				DEBUG_CHECK(false);
				CORE_ERROR("RenderGraph: invalid resolution: {}x{}, node: {}", node.m_width, node.m_height, node.m_name);
				return;
			}

			if (node.m_output_spec.empty()) {
				DEBUG_CHECK(false);
				CORE_ERROR("RenderGraph: output spec not specified, node: {}", node.m_name);
				return;
			}

			std::vector<Framebuffer::Attachment> attachments;
			for (auto& kv : node.m_output_spec) {
				attachments.push_back(kv.second);
			}

			if (s_cached_framebuffers.find(node.m_name) != s_cached_framebuffers.end()) {
				auto& cache = s_cached_framebuffers[node.m_name];
				if (cache_is_reusable(cache, node)) {
					node.m_output = cache;
					if (cache->get_width() != node.m_width ||
						cache->get_height() != node.m_height) {
						cache->resize(node.m_width, node.m_height);
					}
					continue;
				}
			}

			node.m_output = Framebuffer::create(node.m_width, node.m_height, attachments);
			s_cached_framebuffers[node.m_name] = node.m_output;
		}

		// pass #4: handle passthrough
		for (auto& node : m_nodes) {
			if (!node.m_passthrough_pass.empty()) {
				// passthrought: inherit output from another node
				node.m_output = m_nodes[name_to_id[node.m_passthrough_pass]].m_output;
			}
		}

		// pass #4: build render passes and update w/h
		for (auto& node : m_nodes) {
			node.m_render_pass = std::make_shared<RenderPass>(node.m_pass_desc);
			node.m_width = node.m_output->get_width();
			node.m_height = node.m_output->get_height();
		}

		m_compiled = true;
	}

	void RenderGraph::execute() {
		if (!m_compiled) {
			CORE_WARN("RenderGraph: not compiled yet");
			return;
		}

		auto& ctx = g_runtime_context.m_graphics_context;

		for (auto i : m_exec_order) {
			auto& node = m_nodes[i];

			ctx->push_debug_group(node.m_name);
			ctx->m_stats.begin_counter(node.m_name);

			ctx->bind_framebuffer(node.m_output);
			node.m_render_pass->execute = [&](GraphicsContext& ctx) {
				node.m_exec_func(node, ctx);
				};
			if (node.m_pre_pass_func)
				node.m_pre_pass_func(node, *ctx);
			ctx->exec_render_pass(node.m_render_pass);
			if (node.m_post_pass_func)
				node.m_post_pass_func(node, *ctx);

			ctx->pop_debug_group();
			ctx->m_stats.end_counter();
		}
	}

	void RenderGraph::clear_cache() {
		s_cached_framebuffers.clear();
	}

}
