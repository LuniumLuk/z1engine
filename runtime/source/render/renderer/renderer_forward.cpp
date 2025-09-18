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
		m_default_material = g_runtime_context.m_asset_manager->get<MaterialInstance>(Guid::make("material/MI_mesh_viewer"));
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
		glm::vec3 sun_dir = { 0.577f, 0.577f, 0.577f };
		glm::vec3 sun_intensity = { .5f, .5f, .5f };
		PerFrameConst per_frame{};
		per_frame.projview = projview;
		per_frame.cam_position = cam_pos;
		per_frame.sun_direction = sun_dir;
		per_frame.sun_intensity = sun_intensity;

		auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const>();
		for (auto [entity, transform, mesh] : view.each()) {
			per_frame.model = transform.get_world_transform();
			mesh.m_mesh->draw(per_frame, m_default_material);
		}

		m_render_pass->unbind();
	}

}
