#include "pch.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "core/core.h"
#include "render/renderer/renderer_2d.h"

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
		auto const& renderer_2d = g_runtime_context.m_renderer_2d;
		std::vector<Renderer2D::Quad> quads;

		auto view = m_registry.view<TransformComponent const, SpriteComponent const>();
		for (auto [entity, transform, sprite] : view.each()) {
			Renderer2D::Quad quad{};
			quad.m_transform = transform.get_transform();
			quad.m_color = sprite.m_color;
			quads.push_back(quad);
		}

		renderer_2d->draw_quads(quads);
		renderer_2d->prepare_draw();
		renderer_2d->draw();
	}

}
