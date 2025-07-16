#include "pch.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "scene/component/basic.h"
#include "scene/component/camera.h"
#include "scene/component/mesh.h"
#include "scene/component/sprite.h"
#include "core/core.h"
#include "render/shader.h"
#include "render/renderer/renderer_2d.h"
#include "render/renderer/renderer_mesh_viewer.h"

namespace z1 {

	Scene::Scene() {}

	Scene::~Scene() {
		// when the scene is being destroyed, the weak_ptr to the scene in each entity will be expired
		// that is when calling m_scene.lock() in Entity::get_component<EntityPtr>() will return nullptr
		// thus, we just mark all entities as destroyed, and clear the registry
		// the entities' dtor will avoid using the weak_ptr to the scene
		for (auto& entity : m_entities) {
			if (entity) {
				entity->m_is_destroyed = true;
			}
		}
		m_registry.clear();
	}

	std::shared_ptr<Entity> Scene::create_entity(std::string const& name) {
		entt::entity handle = m_registry.create();
		CORE_INFO("Creating entity {} ({})", name, static_cast<uint32_t>(handle));
		auto entity = std::make_shared<Entity>(handle, shared_from_this());
		entity->add_component<TagComponent>(name);
		entity->add_component<TransformComponent>();
		entity->add_component<Scene::EntityPtr>(entity);
		m_entities.push_back(entity);
		return entity;
	}

	std::shared_ptr<Entity> Scene::cast_to_entity(entt::entity handle) const {
		CORE_ASSERT(m_registry.valid(handle), "Entity handle is invalid!");
		auto entity_ptr = m_registry.try_get<Scene::EntityPtr>(handle);
		if (entity_ptr) {
			return *entity_ptr;
		}
		return nullptr;
	}

	std::shared_ptr<Entity> Scene::get_main_camera() const {
		auto view = m_registry.view<CameraComponent>();
		for (auto& entity : view) {
			auto& camera = view.get<CameraComponent>(entity);
			if (camera.m_is_primary) {
				return cast_to_entity(entity);
			}
		}
		return nullptr;
	}

	void Scene::on_update(float delta_time) {
		PROFILE_FUNCTION();

		auto const& main_cam = get_main_camera();
		if (!main_cam) {
			CORE_ERROR("No main camera found in the scene!");
			return;
		}
		auto& main_cc = main_cam->get_component<CameraComponent>();
		auto& main_ct = main_cam->get_component<TransformComponent>();

		if (!main_cc.m_use_fixed_aspect) {
			main_cc.m_aspect = (float)Framebuffer::get_height(m_main_framebuffer) / (float)Framebuffer::get_width(m_main_framebuffer);
		}

		auto cam_up = main_ct.get_transform() * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
		auto cam_forward = main_ct.get_transform() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		auto cam_view = glm::lookAt(main_ct.m_location, main_ct.m_location + glm::vec3(cam_forward), glm::vec3(cam_up));

		glm::mat4 cam_projview = main_cc.get_proj() * cam_view;
		{
			PROFILE_SCOPE("Renderer Mesh Viewer");

			auto& renderer_mesh_viewer = g_runtime_context.m_renderer_mesh_viewer;
			renderer_mesh_viewer->prepare_draw(m_main_framebuffer);
			renderer_mesh_viewer->m_render_pass->m_shader->set_uniform("u_projview", &cam_projview);
			renderer_mesh_viewer->m_render_pass->m_shader->set_uniform("u_cam_position", &main_ct.m_location);

			glm::vec3 sun_dir = { 0.577f, 0.577f, 0.577f };
			glm::vec3 sun_intensity = { .5f, .5f, .5f };
			renderer_mesh_viewer->m_render_pass->m_shader->set_uniform("u_sun_direction", &sun_dir);
			renderer_mesh_viewer->m_render_pass->m_shader->set_uniform("u_sun_intensity", &sun_intensity);

			auto view = m_registry.view<TransformComponent const, StaticMeshComponent const>();
			for (auto [entity, transform, mesh] : view.each()) {
				Renderer2D::Quad quad{};
				renderer_mesh_viewer->m_render_pass->m_shader->set_uniform("u_model", &transform.get_transform());
				mesh.m_mesh->draw();
			}

			renderer_mesh_viewer->after_draw();
		}

		{
			PROFILE_SCOPE("Renderer 2D");

			auto const& renderer_2d = g_runtime_context.m_renderer_2d;
			std::vector<Renderer2D::Quad> quads;

			auto view = m_registry.view<TransformComponent const, SpriteComponent const>();
			for (auto [entity, transform, sprite] : view.each()) {
				Renderer2D::Quad quad{};
				quad.m_transform = transform.get_transform();
				quad.m_color = sprite.m_color;
				quad.m_texture = sprite.m_texture;
				quad.m_tiling_scale = sprite.m_tiling_scale;
				quad.m_tiling_offset = sprite.m_tiling_offset;
				quad.m_texcoords = sprite.m_texcoords;
				quads.push_back(quad);
			}

			renderer_2d->draw_quads(quads);
			renderer_2d->prepare_draw(m_main_framebuffer);
			renderer_2d->m_render_pass->m_shader->set_uniform("u_projview", &cam_projview);

			renderer_2d->draw();
		}
	}

}
