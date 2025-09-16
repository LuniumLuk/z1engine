#include "pch.h"
#include "render/shader.h"
#include "render/framebuffer.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "scene/component/camera.h"
#include "scene/component/mesh.h"
#include "render/renderer/renderer_forward.h"
#include "asset/asset_manager.h"
#include "glm/gtc/matrix_transform.hpp"

namespace z1 {

	RendererForward::RendererForward() {
		// TODO: temporary, later will be replaced by material system
		Pipeline::Description desc{};
		desc.depth_test = true;
		desc.blend = true;
		desc.cull_mode = CullMode::Back;
		desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/mesh_viewer");
		m_pipeline = Pipeline::build(desc);

		m_render_pass = RenderPass::build();
	}

	RendererForward::~RendererForward() {

	}

	void RendererForward::draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer) {
		PROFILE_FUNCTION();

		RenderPass::BeginInfo info{};
		info.framebuffer = framebuffer;
		info.clear_color = true;
		info.clear_depth = true;
		info.clear_color_value = { 0.1f, 0.1f, 0.1f, 1.0f };
		info.clear_depth_value = 1.0f;
		m_render_pass->bind(info);

		auto const& main_cam = scene->get_main_camera();
		if (!main_cam) {
			m_render_pass->unbind();
			return;
		}

		auto& camera_comp = main_cam->get_component<CameraComponent>();
		if (!camera_comp.m_use_fixed_aspect) {
			camera_comp.m_aspect = (float)Framebuffer::get_width(framebuffer) / (float)Framebuffer::get_height(framebuffer);
		}

		auto projview = camera_comp.get_proj() * camera_comp.get_view();
		auto cam_pos = camera_comp.get_position();

		/*
			main render pass
		*/

		m_pipeline->bind();
		m_pipeline->m_shader->set_uniform("u_projview", &projview);
		m_pipeline->m_shader->set_uniform("u_cam_position", &cam_pos);

		glm::vec3 sun_dir = { 0.577f, 0.577f, 0.577f };
		glm::vec3 sun_intensity = { .5f, .5f, .5f };
		m_pipeline->m_shader->set_uniform("u_sun_direction", &sun_dir);
		m_pipeline->m_shader->set_uniform("u_sun_intensity", &sun_intensity);

		auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const>();
		for (auto [entity, transform, mesh] : view.each()) {
			m_pipeline->m_shader->set_uniform("u_model", &transform.get_world_transform());
			mesh.m_mesh->draw();
		}

		m_pipeline->unbind();
		m_render_pass->unbind();
	}

}
