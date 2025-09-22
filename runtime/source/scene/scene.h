#pragma once

#include "core/core.h"
#include "scene/component/base.h"
#include "scene/component/camera.h"
#include "render/framebuffer.h"
#include "entt.hpp"

namespace z1 {

	struct Entity;

	// Scene is actually a special type of asset that is supposed to inherit from
	// Asset from "asset/asset.h", but since Asset itself is a complex type that
	// depends on many other systems (like asset management, serialization, etc.),
	// we avoid this dependency here to keep the scene system modular and independent.
	struct API Scene : std::enable_shared_from_this<Scene> {
		Scene();
		~Scene();

		void on_update(float delta_time);

		// creates a normal entity that belongs to the scene.
		// - persistent: will be serialized when saving the scene and restored on load.
		// - intended for all game/runtime content: lights, meshes, cameras, scripts, etc.
		std::shared_ptr<Entity> create_entity(std::string const& name);

		// creates a transient (temporary) entity that does NOT belong to the saved scene.
		// - non-persistent: will NOT be serialized, discarded when scene is unloaded.
		// - intended for editor-only or runtime-only helpers (e.g., editor camera, gizmos).
		// - behaves like a normal entity during simulation/rendering, but excluded from serialization.
		std::shared_ptr<Entity> create_transient_entity(std::string const& name);

		std::shared_ptr<Entity> cast_to_entity(entt::entity handle) const;

		size_t get_entity_count() const {
			auto view = m_registry.view<TransformComponent>();
			return view.size();
		}

		void set_main_camera(std::shared_ptr<Entity> const& camera);

		std::shared_ptr<Entity> get_main_camera() const;

		static bool serialize(Filepath const& path, std::shared_ptr<Scene> const& asset);
		static std::shared_ptr<Scene> deserialize(Filepath const& path);

		entt::registry m_registry;
		std::shared_ptr<Framebuffer> m_main_framebuffer;
		std::vector<std::shared_ptr<Entity>> m_entities;
		std::vector<std::shared_ptr<Entity>> m_transient_entities;

	private:
		friend struct Entity;
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

		std::shared_ptr<Entity> create_entity_impl(std::string const& name);

		std::shared_ptr<Entity> m_main_camera = nullptr;
	};

}
