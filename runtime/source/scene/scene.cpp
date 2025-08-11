#include "pch.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "scene/component/base.h"
#include "scene/component/camera.h"
#include "scene/component/mesh.h"
#include "scene/component/sprite.h"
#include "core/core.h"
#include "render/shader.h"
#include "render/renderer/renderer_2d.h"
#include "render/renderer/renderer_forward.h"

namespace z1 {

	Scene::Scene() {}

	Scene::~Scene() {
		auto view = m_registry.view<ScriptComponent>();
		for (auto [entity, script_comp] : view.each()) {
			for (auto& script : script_comp.m_scripts) {
				if (script.instance) {
					script.detach_func(script);
				}
			}
		}

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

	void Scene::set_main_camera(std::shared_ptr<Entity> const& camera) {
		CORE_ASSERT(camera->has_component<CameraComponent>(), "Entity has no CameraComponent!");
		if (camera->get_component<CameraComponent>().m_is_primary) {
			return;
		}

		auto view = m_registry.view<CameraComponent>();
		for (auto& entity : view) {
			auto& camera = view.get<CameraComponent>(entity);
			if (camera.m_is_primary) {
				camera.m_is_primary = false; // unset the previous primary camera
			}
		}

		camera->get_component<CameraComponent>().m_is_primary = true;
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
		auto view = m_registry.view<ScriptComponent>();
		for (auto [entity, script_comp] : view.each()) {
			for (auto it = script_comp.m_scripts.begin(); it != script_comp.m_scripts.end();) {
				auto& script = *it;
				if (!script.instance) {
					script.attach_func(script);
				}
				if (script.instance) {
					script.instance->on_update(delta_time);
					if (!script.instance->is_valid()) {
						script.detach_func(script);
						it = script_comp.m_scripts.erase(it);
					}
					else {
						++it;
					}
				}
			}
		}

		g_runtime_context.m_renderer_forward->draw(shared_from_this(), m_main_framebuffer);
		g_runtime_context.m_renderer_2d->draw(shared_from_this(), m_main_framebuffer);
	}

}
