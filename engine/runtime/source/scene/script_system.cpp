#include "pch.h"
#include "scene/script_system.h"
#include "scene/component/base.h"
#include "python/python_script.h"
#include "render/global.h"

namespace z1 {

	void ScriptSystem::update(Scene* scene, float dt) {
		PROFILE_FUNCTION();

		// Collect entities first to avoid iterator invalidation if scripts modify the registry
		auto view = scene->m_registry.view<ScriptComponent>();
		std::vector<entt::entity> entities;
		entities.reserve(view.size());
		for (auto entity : view) {
			entities.push_back(entity);
		}

		for (auto entity : entities) {
			if (!scene->m_registry.valid(entity)) continue;

			// Initial check if component exists
			if (!scene->m_registry.all_of<ScriptComponent>(entity)) continue;

			// Note: We cannot hold a reference to ScriptComponent across loop iterations
			// because scripts might add/remove components, causing EnTT reallocation.
			// We must re-fetch the component or be very careful.
			// Since we iterate m_scripts by index, let's fetch size first, but size might change!
			// So we loop safely.

			// We need to fetch size at start of iteration, but handle dynamic changes?
			// A simple loop index is safest if we re-fetch the component.

			size_t i = 0;
			while (true) {
				// Re-fetch component on every iteration to handle pool reallocation
				auto* comp = scene->m_registry.try_get<ScriptComponent>(entity);
				if (!comp) break; // Component removed

				if (i >= comp->m_scripts.size()) break;

				// Access via re-fetched pointer
				auto& script_data = comp->m_scripts[i];

				if (!script_data.instance) {
					DEBUG_CHECK(script_data.attach_func);
					script_data.attach_func(script_data);

					DEBUG_CHECK(script_data.instance);
					script_data.instance->m_state = ScriptState::Attached;
				}

				// We want the attach func to be called even if script system is disabled, because it might be
				// needed for editor scripts to function properly.
				if (!g_runtime_context.m_global->script_enabled && !script_data.instance->is_transient())
					break;

				if (script_data.instance->m_state == ScriptState::Attached) {
					// Check if instance was created and call on_start
					script_data.instance->on_start();
					script_data.instance->m_state = ScriptState::Started;
				}

				// Re-fetch again? attach_func might have caused realloc!
				comp = scene->m_registry.try_get<ScriptComponent>(entity);
				if (!comp || i >= comp->m_scripts.size()) break;

				// NOTE: We do NOT define 'current_script' here because using a reference to a vector element
				// that might be erased or reallocated is dangerous if we don't use it immediately.
				// Instead, we access it directly via index when needed.

				// Check validity BEFORE update? No, update might make it invalid.
				// But we also need to check if it's attached.
				if (comp->m_scripts[i].instance) {
					comp->m_scripts[i].instance->on_update(dt);

					// Re-fetch again! on_update might have caused realloc!
					comp = scene->m_registry.try_get<ScriptComponent>(entity);
					if (!comp || i >= comp->m_scripts.size()) break;

					// Check validity
					auto& current = comp->m_scripts[i];
					if (current.instance && !current.instance->is_valid()) {
						current.instance->on_destroy(); // Call on_destroy before detach
						current.instance->m_state = ScriptState::Destroyed;

						if (current.detach_func) {
							current.detach_func(current);
							current.instance->m_state = ScriptState::Detached;
						}
						// Reset unique_ptr explicitly before erase
						current.instance.reset();
						comp->m_scripts.erase(comp->m_scripts.begin() + i);
						// i stays same
					}
					else {
						++i;
					}
				}
				else {
					++i;
				}
			}
		}

		scene->flush_pending_destroy_entities();
	}

	void ScriptSystem::shutdown(Scene* scene) {
		PROFILE_FUNCTION();

		// Collect entities first to avoid iterator invalidation if scripts modify the registry
		auto view = scene->m_registry.view<ScriptComponent>();
		std::vector<entt::entity> entities;
		entities.reserve(view.size());
		for (auto entity : view) {
			entities.push_back(entity);
		}

		for (auto entity : entities) {
			if (!scene->m_registry.valid(entity))
				continue;

			// Initial check if component exists
			if (!scene->m_registry.all_of<ScriptComponent>(entity))
				continue;

			auto* comp = scene->m_registry.try_get<ScriptComponent>(entity);
			if (!comp)
				continue; // Component removed

			comp->detach_all();
		}
	}

}
