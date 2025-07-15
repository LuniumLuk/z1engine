#include "pch.h"
#include "render/shader.h"
#include "render/framebuffer.h"
#include "render/renderer/renderer_mesh_viewer.h"
#include "glm/gtc/matrix_transform.hpp"

namespace z1 {

	RendererMeshViewer::RendererMeshViewer() {
		RenderPass::Description desc{};


		desc.depth_test = true;
		desc.blend = true;

		desc.cull_mode = CullMode::Back;

		desc.shader = Shader::create(g_runtime_context.m_file_system->m_engine_dir / "asset/shader/mesh_viewer.glsl");
		m_render_pass = RenderPass::build(desc);
	}

	RendererMeshViewer::~RendererMeshViewer() {

	}

	void RendererMeshViewer::prepare_draw(std::shared_ptr<Framebuffer> const& framebuffer) const {
		PROFILE_FUNCTION();

		RenderPass::BeginInfo info{};
		info.framebuffer = framebuffer;
		info.clear_color = true;
		info.clear_depth = true;
		info.clear_color_value = { 0.1f, 0.1f, 0.1f, 1.0f };
		info.clear_depth_value = 1.0f;

		m_render_pass->begin(info);
	}

	void RendererMeshViewer::after_draw() const {
		PROFILE_FUNCTION();
		m_render_pass->end();
	}

}
