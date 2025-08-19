#pragma once

#include "core/core.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"

namespace z1 {

	struct API TagComponent {
		std::string m_tag;
		uint32_t m_id = 0;
		TagComponent() = default;
		TagComponent(std::string const& tag, uint32_t id) : m_tag(tag), m_id(id) {}
	};

	struct API TransformComponent {
		glm::vec3 m_location{ 0.0f, 0.0f, 0.0f };
		glm::vec3 m_rotation{ 0.0f, 0.0f, 0.0f }; // in degrees (pitch, yaw, roll)
		glm::vec3 m_scale{ 1.0f, 1.0f, 1.0f };
		TransformComponent* m_parent = nullptr;

		TransformComponent() = default;
		TransformComponent(glm::vec3 const& location, glm::vec3 const& rotation, glm::vec3 const& scale) noexcept
			: m_location(location), m_rotation(rotation), m_scale(scale) {
		}

		void set_parent(TransformComponent* parent) {
			m_parent = parent;
		}

		glm::mat4 get_world_transform() const {
			glm::mat4 t = get_local_transform();
			TransformComponent const* parent = m_parent;
			while (parent) {
				t = t * parent->get_local_transform();
				parent = parent->m_parent;
			}
			return t;
		}

		glm::mat4 get_world_rotation() const {
			glm::mat4 t = get_local_rotation();
			TransformComponent const* parent = m_parent;
			while (parent) {
				t = t * parent->get_local_rotation();
				parent = parent->m_parent;
			}
			return t;
		}

		glm::mat4 get_local_transform() const {
			glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_location);
			glm::mat4 rotation = get_local_rotation();
			glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_scale);
			return translation * rotation * scale;
		}

		glm::mat4 get_local_rotation() const {
			glm::mat4 rotation = glm::mat4(1.0f);
			// rotation order: yaw -> pitch -> roll
			rotation = glm::rotate(rotation, glm::radians(m_rotation.y), { 0.0f, 1.0f, 0.0f });
			rotation = glm::rotate(rotation, glm::radians(m_rotation.x), { 1.0f, 0.0f, 0.0f });
			rotation = glm::rotate(rotation, glm::radians(m_rotation.z), { 0.0f, 0.0f, 1.0f });
			return rotation;
		}

		void set_local_transform(glm::mat4 const& transform) {
			glm::vec3 skew{};
			glm::vec4 perspective{};
			glm::quat orientation{};

			if (glm::decompose(transform, m_scale, orientation, m_location, skew, perspective)) {
				glm::vec3 euler = glm::eulerAngles(orientation);
				m_rotation = glm::degrees(euler);
			}
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
			auto params = std::make_tuple(std::forward<Args>(args)...);
			sd.attach_func = [this, params = std::move(params)](ScriptData& d)
				{
					std::apply([&](auto&&... unpacked)
						{
							d.instance = new ScriptType(std::forward<decltype(unpacked)>(unpacked)...);
						}, params);
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
