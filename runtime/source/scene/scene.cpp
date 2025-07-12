#include "pch.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "core/core.h"
#include "render/shader.h"
#include "render/renderer/renderer_2d.h"
#include "render/renderer/renderer_mesh_viewer.h"

namespace z1 {

	Scene::Scene() {}

	Scene::~Scene() {}

	std::shared_ptr<Entity> Scene::create_entity(std::string const& name) {
		entt::entity handle = m_registry.create();
		CORE_INFO("Creating entity {} ({})", name, static_cast<uint32_t>(handle));
		auto entity = std::make_shared<Entity>(handle, shared_from_this());
		entity->add_component<TagComponent>(name);
		entity->add_component<TransformComponent>();
		return entity;
	}

	void Scene::on_update(float delta_time) {
		{
			PROFILE_SCOPE("Renderer Mesh Viewer");

			auto& renderer_mesh_viewer = g_runtime_context.m_renderer_mesh_viewer;
			renderer_mesh_viewer->prepare_draw();

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
			renderer_2d->prepare_draw();
			renderer_2d->draw();
		}
	}

}
