#pragma once

#include "core/core.h"
#include "scene/scene.h"
#include "scene/component/base.h"
#include "entt.hpp"

namespace z1 {

	struct Scene;

	struct API Entity {
		Entity(entt::entity handle, std::weak_ptr<Scene> const& scene)
			: m_handle(handle), m_scene(scene) {
		}

		Entity(Entity const&) = default;
		Entity& operator=(Entity const&) = default;
		Entity(Entity&&) = default;
		Entity& operator=(Entity&&) = default;

		~Entity() {
			// when m_is_destroyed is true, the scene is being destroyed
			// thus we should not try to access the scene and the registry
			if (m_is_destroyed) {
				return;
			}
			CORE_ASSERT(is_valid(), "Entity is invalid!");
			CORE_INFO("destroying entity {} ({})", get_component<TagComponent>().m_tag, static_cast<uint32_t>(m_handle));
			m_scene.lock()->m_registry.destroy(m_handle);
		}

		template<typename T>
		bool has_component() const {
			CORE_ASSERT(is_valid(), "Entity is invalid!");
			return m_scene.lock()->m_registry.all_of<T>(m_handle);
		}

		template<typename T>
		T& get_component() const {
			CORE_ASSERT(is_valid(), "Entity is invalid!");
			CORE_ASSERT(has_component<T>(), "Entity does not have component of type " + std::string(typeid(T).name()) + "!");
			return m_scene.lock()->m_registry.get<T>(m_handle);
		}

		template<typename T, typename... Args>
		T& add_component(Args&&... args) {
			CORE_ASSERT(is_valid(), "Entity is invalid!");
			CORE_ASSERT(!has_component<T>(), "Entity already has component of type " + std::string(typeid(T).name()) + "!");
			return m_scene.lock()->m_registry.emplace<T>(m_handle, std::forward<Args>(args)...);
		}

		template<typename T>
		void remove_component() {
			CORE_ASSERT(is_valid(), "Entity is invalid!");
			CORE_ASSERT(has_component<T>(), "Entity does not have component of type " + std::string(typeid(T).name()) + "!");
			m_scene.lock()->m_registry.remove<T>(m_handle);
		}

		bool is_valid() const {
			return !m_scene.expired() && m_scene.lock()->m_registry.valid(m_handle);
		}

		template<typename T, typename... Args>
		void attach_script(Args&&... args) {
			CORE_ASSERT(is_valid(), "Entity is invalid!");
			auto& entity_ptr = get_component<Scene::EntityPtr>().m_ptr;
			auto& script_comp = m_scene.lock()->m_registry.get_or_emplace<ScriptComponent>(m_handle, entity_ptr);
			script_comp.bind<T>(std::forward<Args>(args)...);
		}

	private:
		friend Scene;

		entt::entity m_handle = entt::null;
		std::weak_ptr<Scene> m_scene;
		bool m_is_destroyed = false;
	};

}
