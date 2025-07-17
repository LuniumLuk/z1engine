#include "pch.h"
#include "render/shader.h"
#include "render/framebuffer.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "scene/component/camera.h"
#include "scene/component/mesh.h"
#include "render/renderer/renderer_forward.h"
#include "glm/gtc/matrix_transform.hpp"

namespace z1 {

	RendererForward::RendererForward() {
		RenderPass::Description desc{};

		desc.depth_test = true;
		desc.blend = true;

		desc.cull_mode = CullMode::Back;

		desc.shader = Shader::create(g_runtime_context.m_file_system->m_engine_dir / "asset/shader/mesh_viewer.glsl");
		m_render_pass = RenderPass::build(desc);
	}

	RendererForward::~RendererForward() {

	}

	void RendererForward::prepare_draw(std::shared_ptr<Framebuffer> const& framebuffer) const {
		PROFILE_FUNCTION();

		RenderPass::BeginInfo info{};
		info.framebuffer = framebuffer;
		info.clear_color = true;
		info.clear_depth = true;
		info.clear_color_value = { 0.1f, 0.1f, 0.1f, 1.0f };
		info.clear_depth_value = 1.0f;

		m_render_pass->begin(info);
	}

	void RendererForward::after_draw() const {
		PROFILE_FUNCTION();
		m_render_pass->end();
	}

	void RendererForward::draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer) {
		PROFILE_FUNCTION();

		auto const& main_cam = scene->get_main_camera();
		if (!main_cam) {
			CORE_ERROR("No main camera found in the scene!");
			return;
		}

		auto& camera_comp = main_cam->get_component<CameraComponent>();
		if (!camera_comp.m_use_fixed_aspect) {
			camera_comp.m_aspect = (float)Framebuffer::get_width(framebuffer) / (float)Framebuffer::get_height(framebuffer);
		}

		auto& camera_trans = main_cam->get_component<TransformComponent>();

		auto cam_up = camera_trans.get_transform() * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
		auto cam_forward = camera_trans.get_transform() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		auto cam_view = glm::lookAt(camera_trans.m_location, camera_trans.m_location + glm::vec3(cam_forward), glm::vec3(cam_up));

		glm::mat4 cam_projview = camera_comp.get_proj() * cam_view;

		prepare_draw(framebuffer);
		m_render_pass->m_shader->set_uniform("u_projview", &cam_projview);
		m_render_pass->m_shader->set_uniform("u_cam_position", &camera_trans.m_location);

		glm::vec3 sun_dir = { 0.577f, 0.577f, 0.577f };
		glm::vec3 sun_intensity = { .5f, .5f, .5f };
		m_render_pass->m_shader->set_uniform("u_sun_direction", &sun_dir);
		m_render_pass->m_shader->set_uniform("u_sun_intensity", &sun_intensity);

		auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const>();
		for (auto [entity, transform, mesh] : view.each()) {
			m_render_pass->m_shader->set_uniform("u_model", &transform.get_transform());
			mesh.m_mesh->draw();
		}

		after_draw();
	}

}
