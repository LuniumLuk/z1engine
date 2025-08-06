#pragma once

#include "core/core.h"

namespace z1 {

	struct Entity;

	struct API ScriptBase {

		virtual void on_create() = 0;
		virtual void on_update(float delta_time) = 0;

		std::shared_ptr<Entity> const& get_entity() {
			return m_entity.lock();
		}

	private:
		friend struct ScriptComponent;
		std::weak_ptr<Entity> m_entity;

	};

	struct API ScriptComponent {
		std::weak_ptr<Entity> m_entity;
		std::vector<std::unique_ptr<ScriptBase>> m_scripts;

		ScriptComponent(ScriptComponent const&) = delete;
		ScriptComponent& operator=(ScriptComponent const&) = delete;

		ScriptComponent(ScriptComponent&&) = default;
		ScriptComponent& operator=(ScriptComponent&&) = default;

		ScriptComponent(std::weak_ptr<Entity> const& entity) noexcept
			: m_entity(entity) {
		}

		template<typename ScriptType, typename... Args>
		void bind(Args&&... args) {
			m_scripts.emplace_back(
				std::make_unique<ScriptType>(std::forward<Args>(args)...)
			);
			m_scripts.back()->m_entity = m_entity;
			m_scripts.back()->on_create();
		}
	};

}
