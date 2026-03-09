#pragma once

#include "core/core.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtx/euler_angles.hpp"

namespace z1 {

	template<typename... Deps>
	struct Requires {
		using requires_tuple = std::tuple<Deps...>;  // marker type

		std::tuple<Deps*...> m_deps{};

		template<typename T>
		void set_dependency(T& dep) {
			std::get<T*>(m_deps) = &dep;
		}

		template<typename T>
		T& get_component() const {
			return *std::get<T*>(m_deps);
		}
	};

	REFLECTED_STRUCT(TagComponent) {
		std::string m_tag;
		uint32_t m_id = 0;
		TagComponent() = default;
		TagComponent(std::string const& tag, uint32_t id) : m_tag(tag), m_id(id) {}

		DISABLE_COPY(TagComponent)
	};

	REFLECTED_FIELD(TagComponent, m_tag, FF_Default)

	REFLECTED_STRUCT(TransformComponent) {
		glm::vec3 m_location{ 0.0f, 0.0f, 0.0f };
		glm::vec3 m_rotation{ 0.0f, 0.0f, 0.0f }; // in degrees (pitch, yaw, roll)
		glm::vec3 m_scale{ 1.0f, 1.0f, 1.0f };
		TransformComponent* m_parent = nullptr;

		TransformComponent() = default;
		TransformComponent(glm::vec3 const& location, glm::vec3 const& rotation, glm::vec3 const& scale) noexcept
			: m_location(location), m_rotation(rotation), m_scale(scale) {
		}

		DISABLE_COPY(TransformComponent)

		void set_parent(TransformComponent* parent) {
			m_parent = parent;
		}

		glm::mat4 get_world_transform() const {
			glm::mat4 t = get_local_transform();
			TransformComponent const* parent = m_parent;
			while (parent) {
				t = parent->get_local_transform() * t;
				parent = parent->m_parent;
			}
			return t;
		}

		glm::mat4 get_world_rotation() const {
			glm::mat4 t = get_local_rotation();
			TransformComponent const* parent = m_parent;
			while (parent) {
				t = parent->get_local_rotation() * t;
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
			// rotation order: roll -> yaw -> pitch
			// as in consistent with ImGuizmo
			rotation = glm::rotate(rotation, glm::radians(m_rotation.z), { 0.0f, 0.0f, 1.0f });
			rotation = glm::rotate(rotation, glm::radians(m_rotation.y), { 0.0f, 1.0f, 0.0f });
			rotation = glm::rotate(rotation, glm::radians(m_rotation.x), { 1.0f, 0.0f, 0.0f });
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

		void set_world_transform(glm::mat4 const& transform) {
			if (m_parent) {
				glm::mat4 parent_world = m_parent->get_world_transform();
				glm::mat4 local_transform = glm::inverse(parent_world) * transform;
				set_local_transform(local_transform);
			}
			else {
				set_local_transform(transform);
			}
		}
	};

	REFLECTED_FIELD(TransformComponent, m_location, FF_Default, "[input]")
	REFLECTED_FIELD(TransformComponent, m_rotation, FF_Default, "[input]")
	REFLECTED_FIELD(TransformComponent, m_scale,    FF_Default, "[input]")

	struct Entity;

#define REGISTER_SCRIPT(ScriptType) \
	std::string get_script_name() const override { return #ScriptType; }

	struct API ScriptBase {

		virtual void on_attach() = 0;
		virtual void on_update(float delta_time) = 0;
		virtual void on_detach() = 0;

		virtual std::string get_script_name() const { return "unregistered"; }

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
			return !m_entity.expired() && m_entity.lock()->is_valid();
		}

		bool is_valid() const {
			return m_is_valid;
		}

		void destroy() {
			m_is_valid = false;
		}

	private:
		friend struct ScriptComponent;
		friend struct PythonScript;
		std::weak_ptr<Entity> m_entity;
		bool m_is_valid = true;

	};

	struct API ScriptComponent {

		struct ScriptData {
			ScriptData() = default;
			ScriptData(ScriptData const&) = delete;
			ScriptData& operator=(ScriptData const&) = delete;

			ScriptData(ScriptData &&) = default;
			ScriptData& operator=(ScriptData &&) = default;

			std::unique_ptr<ScriptBase> instance = nullptr;
			std::function<void(ScriptData&)> attach_func = nullptr;
			std::function<void(ScriptData&)> detach_func = nullptr;
		};

		std::weak_ptr<Entity> m_entity;
		std::vector<ScriptData> m_scripts;

		DISABLE_COPY(ScriptComponent)

		ScriptComponent(ScriptComponent&&) = default;
		ScriptComponent& operator=(ScriptComponent&&) = default;

		ScriptComponent(std::weak_ptr<Entity> const& entity) noexcept
			: m_entity(entity) {
		}

		void detach_all() {
			for (auto& script : m_scripts) {
				if (script.instance) {
					if (script.detach_func) {
						script.detach_func(script);
					}
					script.instance.reset();
				}
			}
			m_scripts.clear();
		}

		~ScriptComponent() {
			detach_all();
		}

		template<typename ScriptType, typename... Args>
		void bind(Args&&... args) {
			ScriptData sd{};
			auto params = std::make_tuple(std::forward<Args>(args)...);

			// CAPTURE FIX: Capture m_entity by value (weak_ptr copy), NOT 'this'
			sd.attach_func = [entity = m_entity, params = std::move(params)](ScriptData& d) mutable
				{
					std::apply([&](auto&&... unpacked)
						{
							d.instance = std::make_unique<ScriptType>(std::forward<decltype(unpacked)>(unpacked)...);
						}, params);
					d.instance->m_entity = entity;
					d.instance->on_attach();
				};

			sd.detach_func = [](ScriptData& d)
				{
					if (d.instance) {
						d.instance->on_detach();
					}
				};

			m_scripts.emplace_back(std::move(sd));
		}

		template<typename ScriptType>
		void unbind() {
			auto it = std::find_if(m_scripts.begin(), m_scripts.end(),
				[](ScriptData const& sd) {
					return dynamic_cast<ScriptType*>(sd.instance.get()) != nullptr;
				});

			if (it != m_scripts.end()) {
				if (it->detach_func) {
					it->detach_func(*it);
				}
				m_scripts.erase(it);
			}
		}

		void unbind_at(size_t index) {
			if (index >= m_scripts.size()) {
				return;
			}

			auto& sd = m_scripts[index];
			if (sd.detach_func) {
				sd.detach_func(sd);
			}
			m_scripts.erase(m_scripts.begin() + index);
		}
	};

}
