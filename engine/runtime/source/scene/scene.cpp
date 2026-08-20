#include "pch.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "scene/component/base.h"
#include "scene/component/camera.h"
#include "scene/component/mesh.h"
#include "scene/component/sprite.h"
#include "scene/component/light.h"
#include "scene/component/sky_light.h"
#include "scene/component/postprocess_volume.h"
#include "scene/component/animation.h"
#include "scene/component/particle.h"
#include "scene/component/collider.h"
#include "scene/component/physics.h"
#include "scene/animation_system.h"
#include "scene/particle_system.h"
#include "scene/postprocess_system.h"
#include "scene/script_system.h"
#include "scene/physics_system.h"
#include "python/python_script.h"
#include "core/core.h"
#include "core/timer.h"
#include "render/global.h"
#include "render/shader.h"
#include "render/renderer/renderer_2d.h"
#include "render/renderer/renderer_forward.h"
#include "asset/asset_manager.h"
#include "scene/serialization.h"

namespace z1 {

	// Reflection registration for EditorCameraData (manual, nested struct in Scene)
	// Placed in .cpp to avoid duplicate registrations from multiple TUs including scene.h
	struct _REFLECT_REGISTER_EditorCameraData {
		_REFLECT_REGISTER_EditorCameraData() {
			TypeRegistry::instance().register_type("EditorCameraData");

			FieldInfo transform_field = {};
			transform_field.name = "transform";
			transform_field.offset = offsetof(Scene::EditorCameraData, transform);
			transform_field.size = sizeof(TransformComponent);
			transform_field.type = &typeid(TransformComponent);
			transform_field.flag = FF_Default;
			TypeRegistry::instance().register_field("EditorCameraData", transform_field);

			FieldInfo camera_field = {};
			camera_field.name = "camera";
			camera_field.offset = offsetof(Scene::EditorCameraData, camera);
			camera_field.size = sizeof(CameraComponent);
			camera_field.type = &typeid(CameraComponent);
			camera_field.flag = FF_Default;
			TypeRegistry::instance().register_field("EditorCameraData", camera_field);

			FieldInfo valid_field = {};
			valid_field.name = "is_valid";
			valid_field.offset = offsetof(Scene::EditorCameraData, is_valid);
			valid_field.size = sizeof(bool);
			valid_field.type = &typeid(bool);
			valid_field.flag = FF_ReadOnly;
			TypeRegistry::instance().register_field("EditorCameraData", valid_field);
		}
	};
	static _REFLECT_REGISTER_EditorCameraData _REFLECT_REGISTER_INSTANCE_EditorCameraData;

	Scene::Scene() {}

	Scene::~Scene() {
		ScriptSystem::shutdown(this);

		// when the scene is being destroyed, the weak_ptr to the scene in each entity will be expired
		// that is when calling m_scene.lock() in Entity::get_component<EntityPtr>() will return nullptr
		// thus, we just mark all entities as destroyed, and clear the registry
		// the entities' dtor will avoid using the weak_ptr to the scene
		for (auto& entity : m_entities) {
			if (entity) entity->m_is_destroyed = true;
		}
		for (auto& entity : m_transient_entities) {
			if (entity) entity->m_is_destroyed = true;
		}

		m_registry.clear();
	}

	std::shared_ptr<Entity> Scene::create_entity_impl(std::string const& name) {
		entt::entity handle = m_registry.create();
		CORE_DEBUG("creating entity {} ({})", name, static_cast<uint32_t>(handle));
		auto entity = std::make_shared<Entity>(handle, shared_from_this());
		entity->add_component<TagComponent>(name, static_cast<uint32_t>(m_entities.size()));
		entity->add_component<TransformComponent>();
		entity->add_component<Scene::EntityPtr>(entity);
		return entity;
	}

	std::shared_ptr<Entity> Scene::create_entity(std::string const& name) {
		auto entity = create_entity_impl(name);
		m_entities.push_back(entity);
		mark_dirty();
		return entity;
	}

	std::shared_ptr<Entity> Scene::create_transient_entity(std::string const& name) {
		auto entity = create_entity_impl(name);
		m_transient_entities.push_back(entity);
		return entity;
	}

	void Scene::destroy_entity(std::shared_ptr<Entity> const& entity) {
		if (!entity || !entity->is_valid()) return;

		entity->m_is_destroyed = true;
		m_pending_destroy_entities.push_back(entity);
	}

	void Scene::flush_pending_destroy_entities() {
		for (auto& entity : m_pending_destroy_entities) {
			if (!entity) continue;

			// Manually detach scripts first to ensure they can run cleanup logic
			// while the entity and its components are still valid.
			//if (entity->has_component<ScriptComponent>()) {
			//	entity->get_component<ScriptComponent>().detach_all();
			//}

			// Force destruction of the underlying entity in the registry
			// This ensures components are destroyed even if Python holds a shared_ptr
			m_registry.destroy(entity->m_handle);

			auto it = std::find(m_entities.begin(), m_entities.end(), entity);
			if (it != m_entities.end()) {
				m_entities.erase(it);
				mark_dirty();
				continue;
			}

			auto it_transient = std::find(m_transient_entities.begin(), m_transient_entities.end(), entity);
			if (it_transient != m_transient_entities.end()) {
				m_transient_entities.erase(it_transient);
				// mark_dirty(); // transient entities don't affect scene dirtiness?
			}
		}
		m_pending_destroy_entities.clear();
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
		m_main_camera = camera;
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
		mark_dirty();
	}

	std::shared_ptr<Entity> Scene::get_main_camera() const {
		return m_main_camera;
	}

	void Scene::on_update(float delta_time) {
		PROFILE_FUNCTION();

		// Save previous frame's transform for TAA/Motion Blur
		auto view = m_registry.view<TransformComponent>();
		for (auto entity : view) {
			auto& tc = view.get<TransformComponent>(entity);
			tc.m_prev_world_transform = tc.get_world_transform();
		}

		PostProcessSystem::update(this);
	}

	void Scene::on_fixed_update() {
		AnimationSystem::update(this, Timer::fixed_update_delta);
		ParticleSystem::update(this, Timer::fixed_update_delta);
		ScriptSystem::update(this, Timer::fixed_update_delta);
#ifdef PLATFORM_WINDOWS
		PhysicsSystem::update(this, Timer::fixed_update_delta);
#endif
	}

	std::shared_ptr<Scene> Scene::create(Filepath const& path) {
		auto scene = std::make_shared<Scene>();

		scene->m_meta.guid = Guid::generate();
		scene->m_meta.type = "scene";

		auto [root_name, sub_path] = g_runtime_context.m_asset_manager->resolve_asset_path(path, PathResolveMode::Create);

		scene->m_meta.root = root_name;
		scene->m_meta.path = sub_path;
		scene->mark_dirty();

		auto root = FileSystem::get_root_path(root_name);
		if (g_runtime_context.m_asset_manager->register_asset(scene->m_meta, root)) {
			CORE_DEBUG("created new scene: {}", path.generic_string());
		}
		else {
			scene.reset();
			CORE_ERROR("failed to create scene: {}", path.generic_string());
		}

		return scene;
	}

	std::shared_ptr<Scene> Scene::load(Guid const& guid, AssetMeta const& meta, Filepath const& file) {
		auto scene = std::make_shared<Scene>();
		scene->m_meta = meta;
		scene->mark_saved();

		YAML::Node yaml;
		try {
			yaml = YAML::LoadFile(concat(file, ".yaml").string());
		}
		catch (YAML::ParserException& e) {
			CORE_ERROR("failed to load scene file: {0}, {1}", file.generic_string(), e.what());
			return scene;
		}

		if (!yaml["meta"] || !yaml["meta"]["type"] || yaml["meta"]["type"].as<std::string>() != "scene") {
			CORE_ERROR("not a scene file: {}", file.generic_string());
			return scene;
		}

		// Global Settings (reflection-driven)
		auto global_settings = yaml["global_settings"];
		if (global_settings) {
			auto& global = *g_runtime_context.m_global;
			deserialize_type(global_settings, &global, "GlobalSettings");
		}

		// Editor Camera
		auto editor_camera = yaml["editor_camera"];
		if (editor_camera) {
			scene->m_editor_camera_data.is_valid = true;

			auto transform_yaml = editor_camera["transform"];
			scene->m_editor_camera_data.transform.m_location = transform_yaml["location"].as<glm::vec3>();
			scene->m_editor_camera_data.transform.m_rotation = transform_yaml["rotation"].as<glm::vec3>();
			scene->m_editor_camera_data.transform.m_scale = transform_yaml["scale"].as<glm::vec3>();

			auto camera_yaml = editor_camera["camera"];
			scene->m_editor_camera_data.camera.m_is_perspective = camera_yaml["is_perspective"].as<bool>();
			scene->m_editor_camera_data.camera.m_intrinsic.fov = camera_yaml["intrinsic"].as<float>();
			scene->m_editor_camera_data.camera.m_near = camera_yaml["near"].as<float>();
			scene->m_editor_camera_data.camera.m_far = camera_yaml["far"].as<float>();
			scene->m_editor_camera_data.camera.m_aspect = camera_yaml["aspect"].as<float>();
			scene->m_editor_camera_data.camera.m_use_fixed_aspect = camera_yaml["use_fixed_aspect"].as<bool>();
		}

		auto entities = yaml["entities"];
		if (entities) {
			scene->create_entities_from_yaml(entities);
		}
		else {
			CORE_WARN("scene has no entities: {}", file.generic_string());
		}

		return scene;
	}

	std::vector<std::shared_ptr<Entity>> Scene::create_entities_from_yaml(YAML::Node const& entities) {
		std::vector<std::shared_ptr<Entity>> created_entities;
		std::unordered_map<uint32_t, TransformComponent*> id_to_transform;
		std::vector<std::pair<TransformComponent*, uint32_t>> transform_parent_pairs;

		// Build YAML key -> component TypeInfo lookup for all registered components
		std::unordered_map<std::string, const TypeInfo*> yaml_key_to_component;
		for (auto const* comp_type : TypeRegistry::instance().get_all_components()) {
			std::string yk = type_to_yaml_key(comp_type->name);
			if (!yk.empty()) {
				yaml_key_to_component[yk] = comp_type;
			}
		}

		for (auto const& entity_yaml : entities) {
			auto entity = create_entity(entity_yaml["name"].as<std::string>());
			created_entities.push_back(entity);

			// Map the ID from YAML (local to file) to the component
			if (entity_yaml["id"]) {
				id_to_transform[entity_yaml["id"].as<uint32_t>()] = &entity->get_component<TransformComponent>();
			}

			// TransformComponent: deserialize reflected fields, then resolve parent
			auto& transform = entity->get_component<TransformComponent>();
			auto const& transform_yaml = entity_yaml["transform"];
			if (transform_yaml) {
				deserialize_type(transform_yaml, &transform, "TransformComponent");
				if (transform_yaml["parent"] && !transform_yaml["parent"].IsNull()) {
					transform_parent_pairs.push_back({ &transform, transform_yaml["parent"].as<uint32_t>() });
				}
			}

			// CameraComponent: deserialize + handle is_primary special case
			if (entity_yaml["camera"]) {
				entity->add_component<CameraComponent>();
				void* cam_ptr = nullptr;
				auto const* cam_info = TypeRegistry::instance().get("CameraComponent");
				if (cam_info && cam_info->get_from) {
					cam_ptr = cam_info->get_from(*entity);
				}
				if (cam_ptr) {
					deserialize_type(entity_yaml["camera"], cam_ptr, "CameraComponent");
				}
				// m_is_primary is FF_ReadOnly (not auto-serialized), so read it manually
				auto& camera = entity->get_component<CameraComponent>();
				if (entity_yaml["camera"]["is_primary"]) {
					camera.m_is_primary = entity_yaml["camera"]["is_primary"].as<bool>();
				}
				if (camera.m_is_primary) {
					if (m_main_camera) {
						CORE_WARN("scene has multiple primary cameras, overriding previous primary camera");
					}
					set_main_camera(entity);
				}
			}

			// ScriptComponent: special - "script_component" key with list of "module.Class" strings
			if (entity_yaml["script_component"]) {
				auto const& scripts_yaml = entity_yaml["script_component"];
				for (auto const& script_entry : scripts_yaml) {
					std::string script_full_name = script_entry.as<std::string>();
					size_t last_dot = script_full_name.find_last_of('.');
					if (last_dot != std::string::npos) {
						std::string module_name = script_full_name.substr(0, last_dot);
						std::string class_name = script_full_name.substr(last_dot + 1);
						entity->attach_script<PythonScript>(module_name, class_name);
					}
					else {
						CORE_WARN("Invalid script name format: {}. Expected module.Class", script_full_name);
					}
				}
			}

			// All other components: fully reflection-driven
			// Iterate YAML keys of this entity and match against known component types.
			for (auto yaml_it = entity_yaml.begin(); yaml_it != entity_yaml.end(); ++yaml_it) {
				std::string key = yaml_it->first.as<std::string>();
				// Skip keys we handle specially
				if (key == "name" || key == "id" || key == "transform"
					|| key == "camera" || key == "script_component") {
					continue;
				}

				auto comp_it = yaml_key_to_component.find(key);
				if (comp_it == yaml_key_to_component.end()) continue;

				auto const* comp_type = comp_it->second;
				std::string const& type_name = comp_type->name;

				// Skip types already handled
				if (type_name == "TagComponent" || type_name == "TransformComponent"
					|| type_name == "CameraComponent" || type_name == "ScriptComponent") {
					continue;
				}

				// Add the component (uses default/nullary constructor via add_to hook)
				if (comp_type->add_to) {
					comp_type->add_to(*entity);
				}

				// Deserialize fields
				void* comp_ptr = comp_type->get_from
					? comp_type->get_from(*entity)
					: nullptr;
				if (comp_ptr) {
					deserialize_type(yaml_it->second, comp_ptr, type_name);
				}
			}
		}

		// resolve parent references
		for (auto const& [transform_ptr, parent_id] : transform_parent_pairs) {
			if (id_to_transform.find(parent_id) != id_to_transform.end()) {
				transform_ptr->m_parent = id_to_transform[parent_id];
			}
		}

		return created_entities;
	}

	void Scene::save() const {
		// map transform pointer to entity ID for parent reference
		std::unordered_map<void*, uint32_t> transform_ptr_to_id;
		transform_ptr_to_id[nullptr] = INVALID_INDEX;
		for (auto const& entity : m_entities) {
			transform_ptr_to_id[&entity->get_component<TransformComponent>()] = entity->get_component<TagComponent>().m_id;
		}

		YAML::Emitter yaml;

		yaml << YAML::BeginMap;

		yaml << YAML::Key << "meta" << YAML::Value << m_meta;

		// Global Settings (reflection-driven)
		auto& global = *g_runtime_context.m_global;
		yaml << YAML::Key << "global_settings" << YAML::Value;
		serialize_type(yaml, &global, "GlobalSettings");

		// Editor Camera
		for (auto const& entity : m_transient_entities) {
			if (entity->get_component<TagComponent>().m_tag == "[Editor] Viewport Camera") {
				auto const& transform = entity->get_component<TransformComponent>();
				auto const& camera = entity->get_component<CameraComponent>();

				yaml << YAML::Key << "editor_camera" << YAML::Value;
				yaml << YAML::BeginMap;

				// Transform
				yaml << YAML::Key << "transform" << YAML::Value;
				yaml << YAML::BeginMap;
				yaml << YAML::Key << "location" << YAML::Value << transform.m_location;
				yaml << YAML::Key << "rotation" << YAML::Value << transform.m_rotation;
				yaml << YAML::Key << "scale" << YAML::Value << transform.m_scale;
				yaml << YAML::EndMap;

				// Camera
				yaml << YAML::Key << "camera" << YAML::Value;
				yaml << YAML::BeginMap;
				yaml << YAML::Key << "is_perspective" << YAML::Value << camera.m_is_perspective;
				yaml << YAML::Key << "intrinsic" << YAML::Value << camera.m_intrinsic.fov;
				yaml << YAML::Key << "near" << YAML::Value << camera.m_near;
				yaml << YAML::Key << "far" << YAML::Value << camera.m_far;
				yaml << YAML::Key << "aspect" << YAML::Value << camera.m_aspect;
				yaml << YAML::Key << "use_fixed_aspect" << YAML::Value << camera.m_use_fixed_aspect;
				yaml << YAML::EndMap;

				yaml << YAML::EndMap;
				break;
			}
		}

		yaml << YAML::Key << "entities" << YAML::Value;
		yaml << YAML::BeginSeq;

		for (auto const& entity : m_entities) {

			yaml << YAML::BeginMap;

			// TagComponent fields promoted to entity level
			auto const& tag = entity->get_component<TagComponent>();
			yaml << YAML::Key << "name" << YAML::Value << tag.m_tag;
			yaml << YAML::Key << "id" << YAML::Value << tag.m_id;

			// TransformComponent: serialize reflected fields, then add parent (special)
			auto const& transform = entity->get_component<TransformComponent>();
			yaml << YAML::Key << "transform" << YAML::Value;
			yaml << YAML::BeginMap;
			auto const* transform_info = TypeRegistry::instance().get("TransformComponent");
			if (transform_info) {
				for (auto const& field : transform_info->fields) {
					if ((field.flag & FF_Serializable) == 0) continue;
					serialize_field(yaml, const_cast<TransformComponent*>(&transform), field);
				}
			}
			// Parent reference: map raw pointer to entity ID
			yaml << YAML::Key << "parent" << YAML::Value;
			if (transform.m_parent) {
				yaml << transform_ptr_to_id[transform.m_parent];
			}
			else {
				yaml << YAML::Null;
			}
			yaml << YAML::EndMap; // transform

			// CameraComponent: special handling for is_primary restoration
			if (entity->has_component<CameraComponent>()) {
				auto const& camera = entity->get_component<CameraComponent>();
				yaml << YAML::Key << "camera" << YAML::Value;
				yaml << YAML::BeginMap;
				auto const* cam_info = TypeRegistry::instance().get("CameraComponent");
				if (cam_info) {
					for (auto const& field : cam_info->fields) {
						if ((field.flag & FF_Serializable) == 0) continue;
						serialize_field(yaml, const_cast<CameraComponent*>(&camera), field);
					}
				}
				// Write is_primary separately (FF_ReadOnly, not auto-serialized); restore editor-takeover value
				yaml << YAML::Key << "is_primary" << YAML::Value
					 << (tag.m_id == m_editor_camera_data.origin_main_camera_id);
				yaml << YAML::EndMap; // camera
			}

			// All other components: fully reflection-driven
			for (auto const* comp_type : TypeRegistry::instance().get_all_components()) {
				std::string const& type_name = comp_type->name;
				if (type_name == "TagComponent" || type_name == "TransformComponent"
					|| type_name == "CameraComponent" || type_name == "ScriptComponent") {
					continue; // handled specially above or below
				}
				if (!comp_type->has_in || !comp_type->has_in(*entity)) continue;

				yaml << YAML::Key << type_to_yaml_key(type_name) << YAML::Value;
				void* comp_ptr = comp_type->get_from
					? comp_type->get_from(*const_cast<Entity*>(entity.get()))
					: nullptr;
				if (comp_ptr) {
					serialize_type(yaml, comp_ptr, type_name);
				}
			}

			// ScriptComponent: special - emit as "script_component" with script names list
			if (entity->has_component<ScriptComponent>()) {
				auto& sc = entity->get_component<ScriptComponent>();
				yaml << YAML::Key << "script_component" << YAML::Value << YAML::BeginSeq;
				for (auto& script : sc.m_scripts) {
					if (script.instance) {
						std::string name = script.instance->get_script_name();
						if (name != "unregistered") {
							yaml << name;
						}
					}
				}
				yaml << YAML::EndSeq;
			}

			yaml << YAML::EndMap; // entity
		}
		yaml << YAML::EndSeq;
		yaml << YAML::EndMap;

		auto root = FileSystem::get_root_path(m_meta.root);
		save_yaml((root / m_meta.path).concat(".yaml"), yaml);

		mark_saved();
	}

}
