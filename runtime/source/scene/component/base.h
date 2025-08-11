#pragma once

#include "core/core.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

namespace z1 {

	struct API TagComponent {
		std::string m_tag;
		TagComponent() = default;
		TagComponent(std::string const& tag) : m_tag(tag) {}
	};

	struct API TransformComponent {
		glm::vec3 m_location{ 0.0f, 0.0f, 0.0f };
		glm::vec3 m_rotation{ 0.0f, 0.0f, 0.0f }; // in degrees (pitch, yaw, roll)
		glm::vec3 m_scale{ 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(glm::vec3 const& location, glm::vec3 const& rotation, glm::vec3 const& scale) noexcept
			: m_location(location), m_rotation(rotation), m_scale(scale) {
		}

		glm::mat4 get_transform() const {
			glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_location);
			glm::mat4 rotation = get_rotation();
			glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_scale);
			return translation * rotation * scale;
		}

		glm::mat4 get_rotation() const {
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.y), { 0.0f, 1.0f, 0.0f }); // yaw
			rotation = glm::rotate(rotation, glm::radians(m_rotation.x), { 1.0f, 0.0f, 0.0f }); // pitch
			rotation = glm::rotate(rotation, glm::radians(m_rotation.z), { 0.0f, 0.0f, 1.0f }); // roll
			return rotation;
		}
	};

	struct Entity;


	struct API ScriptBase {

		virtual void on_attach() = 0;
		virtual void on_update(float delta_time) = 0;
		virtual void on_detach() = 0;

		template<typename T>
		bool has_component() const {
			return m_entity.lock()->has_component<T>();
		}

		template<typename T>
		T& get_component() const {
			return m_entity.lock()->get_component<T>();
		}

		template<typename T, typename... Args>
		T& add_component(Args&&... args) {
			return m_entity.lock()->add_component<T>(std::forward<Args>(args)...);
		}

		template<typename T>
		void remove_component() {
			return m_entity.lock()->remove_component<T>();
		}

		template <typename T = void>
		// we use a template here to ensure that the function compiles correctly.
		// without this template, the compiler would be unable to instantiate the function
		// because the full definition of 'Entity' is not available at this point.
		// The 'Entity' used in this context is only a forward declaration, so the compiler
		// needs the template to delay the function instantiation until the full type definition
		// of 'Entity' is accessible, allowing the function to work properly when the definition is known.
		bool is_entity_valid() const {
			return m_entity.lock()->is_valid();
		}

		bool is_valid() const {
			return m_is_valid;
		}

		void destroy() {
			m_is_valid = false;
		}

	private:
		friend struct ScriptComponent;
		std::weak_ptr<Entity> m_entity;
		bool m_is_valid = true;

	};

	struct API ScriptComponent {

		struct ScriptData {
			ScriptData(ScriptData const&) = delete;
			ScriptData& operator=(ScriptData const&) = delete;

			ScriptData(ScriptData &&) = default;
			ScriptData& operator=(ScriptData &&) = default;

			ScriptBase* instance = nullptr;
			std::function<void(ScriptData&)> attach_func = nullptr;
			std::function<void(ScriptData&)> detach_func = nullptr;
		};

		std::weak_ptr<Entity> m_entity;
		std::vector<ScriptData> m_scripts;

		ScriptComponent(ScriptComponent const&) = delete;
		ScriptComponent& operator=(ScriptComponent const&) = delete;

		ScriptComponent(ScriptComponent&&) = default;
		ScriptComponent& operator=(ScriptComponent&&) = default;

		ScriptComponent(std::weak_ptr<Entity> const& entity) noexcept
			: m_entity(entity) {
		}

		template<typename ScriptType, typename... Args>
		void bind(Args&&... args) {
			ScriptData sd{};
			sd.attach_func = [this](ScriptData& d)
				{
					d.instance = new ScriptType(std::forward<Args>(args)...);
					d.instance->m_entity = m_entity;
					d.instance->on_attach();
				};

			sd.detach_func = [](ScriptData& d)
				{
					d.instance->on_detach();
					delete d.instance;
				};

			m_scripts.emplace_back(std::forward<ScriptData>(sd));
		}
	};

}
