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

	REFLECTED_COMPONENT(TagComponent) {
		std::string m_tag;
		uint32_t m_id = 0;
		TagComponent() = default;
		TagComponent(std::string const& tag, uint32_t id) : m_tag(tag), m_id(id) {}

		DISABLE_COPY(TagComponent)
	};

	REFLECTED_FIELD(TagComponent, m_tag, FF_Default)
	REFLECTED_FIELD(TagComponent, m_id,  FF_ReadOnly)

	REFLECTED_COMPONENT(TransformComponent) {
		glm::vec3 m_location{ 0.0f, 0.0f, 0.0f };
		glm::vec3 m_rotation{ 0.0f, 0.0f, 0.0f }; // in degrees (pitch, yaw, roll)
		glm::vec3 m_scale{ 1.0f, 1.0f, 1.0f };
		TransformComponent* m_parent = nullptr;
		glm::mat4 m_prev_world_transform{ 1.0f };

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

	enum struct ScriptState : int {
		None,
		Attached,
		Started,
		Destroyed,
		Detached,
	};

	struct API ScriptBase {

		virtual ~ScriptBase() = default;

		virtual void on_attach() = 0;
		virtual void on_start() {}
		virtual void on_update(float delta_time) = 0;
		virtual void on_destroy() {}
		virtual void on_detach() = 0;

		virtual std::string get_script_name() const { return "unregistered"; }

		template<typename T, typename Dummy = void>
		bool has_component() const;

		template<typename T, typename Dummy = void>
		T& get_component() const;

		template<typename T, typename... Args, typename Dummy = void>
		T& add_component(Args&&... args);

		template<typename T, typename Dummy = void>
		void remove_component();

		template <typename T = void>
		bool is_entity_valid() const;

		bool is_valid() const {
			return m_is_valid;
		}

		void destroy() {
			m_is_valid = false;
		}

		// Helper for friends to access the entity
		template<typename Dummy = void>
		std::shared_ptr<Entity> get_entity() const;

		void set_entity(std::weak_ptr<void> const& entity) {
			m_entity = entity;
		}

	private:
		friend struct ScriptSystem;
		friend struct ScriptComponent;
		friend struct PythonScript;
		std::weak_ptr<void> m_entity;
		bool m_is_valid = true;
		ScriptState m_state = ScriptState::None;

		// when marked transient, the script will not be controlled by GlobalSettings::script_enabled
		virtual bool is_transient() const { return false; }

	};

	REFLECTED_STRUCT(ScriptComponent) {

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

		// Custom accessor for script entries (module.class string pairs)
		// Registered via a static helper below

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
					script.instance->set_entity(std::weak_ptr<void>());
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

	// Custom accessor field for ScriptComponent script entries
	// Exposes script names as a vector<string> of "module.Class" entries
	struct _REFLECT_REGISTER_ScriptComponent_script_entries {
		_REFLECT_REGISTER_ScriptComponent_script_entries() {
			FieldInfo field_info = {};
			field_info.name = "script_entries";
			field_info.type = &typeid(std::vector<std::string>);
			field_info.flag = FF_Default;
			field_info.offset = 0;
			field_info.size = sizeof(std::vector<std::string>);

			// Custom getter: collect script names from m_scripts
			field_info.custom_getter = [](void* instance) -> void* {
				auto* comp = static_cast<ScriptComponent*>(instance);
				static thread_local std::vector<std::string> entries;
				entries.clear();
				for (auto& script : comp->m_scripts) {
					if (script.instance) {
						std::string name = script.instance->get_script_name();
						if (name != "unregistered") {
							entries.push_back(name);
						}
					}
				}
				return &entries;
			};

			// Custom setter: accepts new entries but attachment requires Entity access
			// Serialization uses this getter; deserialization handles ScriptComponent
			// specially because script instantiation requires the Entity
			field_info.yaml_key = "script_component";

			TypeRegistry::instance().register_field("ScriptComponent", field_info);
		}
	};
	static _REFLECT_REGISTER_ScriptComponent_script_entries _REFLECT_REGISTER_INSTANCE_ScriptComponent_script_entries;

}
