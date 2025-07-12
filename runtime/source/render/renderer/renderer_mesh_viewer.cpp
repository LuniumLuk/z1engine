#include "pch.h"
#include "render/shader.h"
#include "render/camera.h"
#include "render/framebuffer.h"
#include "render/renderer/renderer_mesh_viewer.h"
#include "glm/gtc/matrix_transform.hpp"

namespace z1 {

	RendererMeshViewer::RendererMeshViewer() {
		RenderPass::Description desc{};
		desc.m_clear_color = true;
		desc.m_clear_depth = true;
		desc.m_clear_color_value = { 0.1f, 0.1f, 0.1f, 1.0f };
		desc.m_clear_depth_value = 1.0f;

		desc.m_depth_test = true;
		desc.m_blend = true;

		desc.m_cull_mode = CullMode::Back;

		desc.m_framebuffer = nullptr;
		desc.m_shader = Shader::create(g_runtime_context.m_file_system->m_engine_dir / "asset/shader/mesh_viewer.glsl");
		m_render_pass = RenderPass::build(desc);
	}

	RendererMeshViewer::~RendererMeshViewer() {

	}

	void RendererMeshViewer::prepare_draw() const {
		PROFILE_FUNCTION();
		if (!g_runtime_context.m_main_camera) return;

		m_render_pass->bind();
		g_runtime_context.m_main_camera->set_aspect((float)m_render_pass->m_framebuffer->get_description().m_height / (float)m_render_pass->m_framebuffer->get_description().m_width);
	
		m_render_pass->m_shader->set_uniform("u_projview", &g_runtime_context.m_main_camera->get_projview());
		m_render_pass->m_shader->set_uniform("u_cam_position", &g_runtime_context.m_main_camera->get_eye());
	}

	void RendererMeshViewer::after_draw() const {
		PROFILE_FUNCTION();
		m_render_pass->unbind();
	}

}
