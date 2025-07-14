#pragma once

#include "core/core.h"
#include "scene/component.h"
#include "render/camera.h"
#include "render/framebuffer.h"
#include "entt.hpp"

namespace z1 {

	struct Entity;

	struct API Scene : std::enable_shared_from_this<Scene> {
		Scene();
		~Scene();

		void on_update(float delta_time);

		std::shared_ptr<Entity> create_entity(std::string const& name);
		std::shared_ptr<Entity> cast_to_entity(entt::entity handle);

		size_t get_entity_count() const {
			auto view = m_registry.view<TransformComponent>();
			return view.size_hint();
		}

		entt::registry m_registry;
		std::shared_ptr<Camera> m_main_camera;
		std::shared_ptr<Framebuffer> m_main_framebuffer;

	private:
		struct EntityPtr {
			std::weak_ptr<Entity> m_ptr;

			EntityPtr() = default;
			EntityPtr(std::shared_ptr<Entity> const& entity)
				: m_ptr(entity) {
			}

			EntityPtr(EntityPtr const&) = default;
			EntityPtr& operator=(EntityPtr const&) = default;
			EntityPtr(EntityPtr&&) = delete;
			EntityPtr& operator=(EntityPtr&&) = delete;

			~EntityPtr() = default;

			operator std::shared_ptr<Entity>() const {
				return m_ptr.lock();
			}
		};
	};

}
