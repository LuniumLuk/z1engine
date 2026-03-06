#include "pch.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "scene/component/base.h"
#include "scene/component/camera.h"
#include "scene/component/mesh.h"
#include "scene/component/sprite.h"
#include "scene/component/light.h"
#include "scene/component/animation.h"
#include "scene/animation_system.h"
#include "core/core.h"
#include "render/global.h"
#include "render/shader.h"
#include "render/renderer/renderer_2d.h"
#include "render/renderer/renderer_forward.h"
#include "asset/asset_manager.h"

namespace z1 {

	Scene::Scene() {}

	Scene::~Scene() {
		auto view = m_registry.view<ScriptComponent>();
		for (auto [entity, script_comp] : view.each()) {
			for (auto& script : script_comp.m_scripts) {
				if (script.instance) {
					script.detach_func(script);
				}
			}
		}

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
		CORE_INFO("creating entity {} ({})", name, static_cast<uint32_t>(handle));
		auto entity = std::make_shared<Entity>(handle, shared_from_this());
		entity->add_component<TagComponent>(name, static_cast<uint32_t>(m_entities.size()));
		entity->add_component<TransformComponent>();
		entity->add_component<Scene::EntityPtr>(entity);
		return entity;
	}

	std::shared_ptr<Entity> Scene::create_entity(std::string const& name) {
		auto entity = create_entity_impl(name);
		m_entities.push_back(entity);
		m_is_dirty = true;
		return entity;
	}

	std::shared_ptr<Entity> Scene::create_transient_entity(std::string const& name) {
		auto entity = create_entity_impl(name);
		m_transient_entities.push_back(entity);
		return entity;
	}

	void Scene::destroy_entity(std::shared_ptr<Entity> const& entity) {
		if (!entity || !entity->is_valid()) return;

		auto it = std::find(m_entities.begin(), m_entities.end(), entity);
		if (it != m_entities.end()) {
			m_entities.erase(it);
			m_is_dirty = true;
			return;
		}

		auto it_transient = std::find(m_transient_entities.begin(), m_transient_entities.end(), entity);
		if (it_transient != m_transient_entities.end()) {
			m_transient_entities.erase(it_transient);
			// m_is_dirty = true; // transient entities don't affect scene dirtiness?
		}
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
		m_is_dirty = true;
	}

	std::shared_ptr<Entity> Scene::get_main_camera() const {
		return m_main_camera;
	}

	void Scene::on_update(float delta_time) {
		PROFILE_FUNCTION();
		AnimationSystem::update(this, delta_time);
		auto view = m_registry.view<ScriptComponent>();
		for (auto [entity, script_comp] : view.each()) {
			for (auto it = script_comp.m_scripts.begin(); it != script_comp.m_scripts.end();) {
				auto& script = *it;
				if (!script.instance) {
					script.attach_func(script);
				}
				if (script.instance) {
					script.instance->on_update(delta_time);
					if (!script.instance->is_valid()) {
						script.detach_func(script);
						it = script_comp.m_scripts.erase(it);
					}
					else {
						++it;
					}
				}
			}
		}
	}

	std::shared_ptr<Scene> Scene::create(Filepath const& path) {
		auto scene = std::make_shared<Scene>();

		scene->m_meta.guid = Guid::generate();
		scene->m_meta.type = "scene";
		scene->m_meta.path = path;
		scene->m_is_dirty = true;

		auto const& root = FileSystem::s_content_root;
		if (g_runtime_context.m_asset_manager->register_asset(scene->m_meta, root)) {
			CORE_INFO("created new scene: {}", path.generic_string());
		}
		else {
			scene.reset();
			CORE_ERROR("failed to create scene: {}", path.generic_string());
		}

		return scene;
	}

	std::shared_ptr<Scene> Scene::load(Guid const& guid) {
		auto scene = std::make_shared<Scene>();
		scene->m_meta = g_runtime_context.m_asset_manager->get_meta(guid);
		scene->m_is_dirty = false;

		YAML::Node yaml;
		auto file = g_runtime_context.m_asset_manager->get_file_from_guid(guid);
		try {
			yaml = YAML::LoadFile((file.concat(".yaml")).string());
		}
		catch (YAML::ParserException& e) {
			CORE_ERROR("failed to load scene file: {0}, {1}", file.generic_string(), e.what());
			return scene;
		}

		if (!yaml["meta"] || !yaml["meta"]["type"] || yaml["meta"]["type"].as<std::string>() != "scene") {
			CORE_ERROR("not a scene file: {}", file.generic_string());
			return scene;
		}

		// Global Settings
		auto global_settings = yaml["global_settings"];
		if (global_settings) {
			auto& global = *g_runtime_context.m_global;
			if (global_settings["sun_direction"]) global.sun_direction = global_settings["sun_direction"].as<glm::vec3>();
			if (global_settings["sun_color"]) global.sun_color = global_settings["sun_color"].as<glm::vec4>();
			if (global_settings["sun_intensity"]) global.sun_intensity = global_settings["sun_intensity"].as<float>();
			if (global_settings["sun_ambient_color"]) global.sun_ambient_color = global_settings["sun_ambient_color"].as<glm::vec4>();
			if (global_settings["sun_ambient_intensity"]) global.sun_ambient_intensity = global_settings["sun_ambient_intensity"].as<float>();
			if (global_settings["taa_enabled"]) global.taa_enabled = global_settings["taa_enabled"].as<bool>();
			if (global_settings["taa_blend"]) global.taa_blend = global_settings["taa_blend"].as<float>();
			if (global_settings["pp_exposure"]) global.pp_exposure = global_settings["pp_exposure"].as<float>();
			if (global_settings["pp_gamma"]) global.pp_gamma = global_settings["pp_gamma"].as<float>();
			if (global_settings["pp_tint"]) global.pp_tint = global_settings["pp_tint"].as<glm::vec4>();
			if (global_settings["sm_near"]) global.sm_near = global_settings["sm_near"].as<float>();
			if (global_settings["sm_far"]) global.sm_far = global_settings["sm_far"].as<float>();
			if (global_settings["sm_ortho_size"]) global.sm_ortho_size = global_settings["sm_ortho_size"].as<float>();
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
			scene->m_editor_camera_data.camera.m_is_primary = camera_yaml["is_primary"].as<bool>();
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

		for (auto const& entity_yaml : entities) {
			// TagComponent
			auto entity = create_entity(entity_yaml["name"].as<std::string>());
			created_entities.push_back(entity);

			// Map the ID from YAML (local to file) to the component
			// Note: We don't overwrite the entity's actual runtime ID here,
			// because create_entity() assigns a new unique runtime ID.
			// This mapping is solely for resolving parent pointers within this batch.
			if (entity_yaml["id"]) {
				id_to_transform[entity_yaml["id"].as<uint32_t>()] = &entity->get_component<TransformComponent>();
			}

			// TransformComponent
			auto& transform = entity->get_component<TransformComponent>();
			auto const& transform_yaml = entity_yaml["transform"];
			transform.m_location = transform_yaml["location"].as<glm::vec3>();
			transform.m_rotation = transform_yaml["rotation"].as<glm::vec3>();
			transform.m_scale = transform_yaml["scale"].as<glm::vec3>();
			if (transform_yaml["parent"] && !transform_yaml["parent"].IsNull()) {
				transform_parent_pairs.push_back({ &transform, transform_yaml["parent"].as<uint32_t>() });
			}

			// CameraComponent
			if (entity_yaml["camera"]) {
				auto const& camera_yaml = entity_yaml["camera"];
				auto& camera = entity->add_component<CameraComponent>();
				camera.m_is_perspective = camera_yaml["is_perspective"].as<bool>();
				camera.m_intrinsic.fov = camera_yaml["intrinsic"].as<float>();
				camera.m_near = camera_yaml["near"].as<float>();
				camera.m_far = camera_yaml["far"].as<float>();
				camera.m_aspect = camera_yaml["aspect"].as<float>();
				camera.m_use_fixed_aspect = camera_yaml["use_fixed_aspect"].as<bool>();
				camera.m_is_primary = camera_yaml["is_primary"].as<bool>();
				if (camera.m_is_primary) {
					if (m_main_camera) {
						CORE_WARN("scene has multiple primary cameras, overriding previous primary camera");
					}
					set_main_camera(entity);
				}
			}

			// StaticMeshComponent
			if (entity_yaml["static_mesh"]) {
				auto const& mesh_yaml = entity_yaml["static_mesh"];
				if (mesh_yaml["guid"] && !mesh_yaml["guid"].IsNull()) {
					auto sm = g_runtime_context.m_asset_manager->get<StaticMesh>(Guid::make(mesh_yaml["guid"].as<std::string>()));
					auto& mesh = entity->add_component<StaticMeshComponent>(sm);
				}
			}

			// SkeletalMeshComponent
			if (entity_yaml["skeletal_mesh"]) {
				auto const& mesh_yaml = entity_yaml["skeletal_mesh"];
				if (mesh_yaml["mesh"] && !mesh_yaml["mesh"].IsNull()) {
					auto sk = g_runtime_context.m_asset_manager->get<SkeletalMesh>(
						Guid::make(mesh_yaml["mesh"].as<std::string>()));
					std::shared_ptr<Skeleton> skel = nullptr;
					if (mesh_yaml["skeleton"] && !mesh_yaml["skeleton"].IsNull()) {
						skel = g_runtime_context.m_asset_manager->get<Skeleton>(
							Guid::make(mesh_yaml["skeleton"].as<std::string>()));
					}
					auto& mesh = entity->add_component<SkeletalMeshComponent>(sk, skel);
				}
			}

			// SpriteComponent
			if (entity_yaml["sprite"]) {
				auto const& sprite_yaml = entity_yaml["sprite"];
				auto& sprite = entity->add_component<SpriteComponent>();
				sprite.m_color = sprite_yaml["color"].as<glm::vec4>();
				if (sprite_yaml["texture"] && !sprite_yaml["texture"].IsNull()) {
					auto tex_guid = Guid::make(sprite_yaml["texture"].as<std::string>());
					sprite.m_texture = g_runtime_context.m_asset_manager->get<Texture2D>(tex_guid);
				}
				sprite.m_tiling_scale = sprite_yaml["tiling_scale"].as<glm::vec2>();
				sprite.m_tiling_offset = sprite_yaml["tiling_offset"].as<glm::vec2>();
				auto const& texcoords_yaml = sprite_yaml["texcoords"];
				for (size_t i = 0; i < 4; ++i) {
					sprite.m_texcoords[i] = texcoords_yaml[i].as<glm::vec2>();
				}
			}

			// LightComponent
			if (entity_yaml["light"]) {
				auto const& light_yaml = entity_yaml["light"];
				auto& light = entity->add_component<LightComponent>();
				light.m_type = light_yaml["type"].as<int>();
				light.m_color = light_yaml["color"].as<glm::vec3>();
				light.m_intensity = light_yaml["intensity"].as<float>();
				light.m_range = light_yaml["range"].as<float>();
				light.m_inner_cone = light_yaml["inner_cone"].as<float>();
				light.m_outer_cone = light_yaml["outer_cone"].as<float>();
				light.m_cast_shadow = light_yaml["cast_shadow"].as<bool>();
			}
		}

		// resolve parent references
		for (auto const& [transform, parent_id] : transform_parent_pairs) {
			if (id_to_transform.find(parent_id) != id_to_transform.end()) {
				transform->m_parent = id_to_transform[parent_id];
			}
			else {
				// Warn only if parent ID was present in this batch
				// For prefabs, if parent ID refers to something outside the prefab, it won't be found here.
				// But standard prefabs should be self-contained or root-level.
				// If a root in prefab has a parent in original scene, that parent won't be in prefab.
				// So we just ignore it (it becomes a root in the new scene).
				// CORE_WARN("failed to find parent with id {}", parent_id);
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

		// Global Settings
		auto& global = *g_runtime_context.m_global;
		yaml << YAML::Key << "global_settings" << YAML::Value;
		yaml << YAML::BeginMap;
		yaml << YAML::Key << "sun_direction" << YAML::Value << global.sun_direction;
		yaml << YAML::Key << "sun_color" << YAML::Value << global.sun_color;
		yaml << YAML::Key << "sun_intensity" << YAML::Value << global.sun_intensity;
		yaml << YAML::Key << "sun_ambient_color" << YAML::Value << global.sun_ambient_color;
		yaml << YAML::Key << "sun_ambient_intensity" << YAML::Value << global.sun_ambient_intensity;
		yaml << YAML::Key << "taa_enabled" << YAML::Value << global.taa_enabled;
		yaml << YAML::Key << "taa_blend" << YAML::Value << global.taa_blend;
		yaml << YAML::Key << "pp_exposure" << YAML::Value << global.pp_exposure;
		yaml << YAML::Key << "pp_gamma" << YAML::Value << global.pp_gamma;
		yaml << YAML::Key << "pp_tint" << YAML::Value << global.pp_tint;
		yaml << YAML::Key << "sm_near" << YAML::Value << global.sm_near;
		yaml << YAML::Key << "sm_far" << YAML::Value << global.sm_far;
		yaml << YAML::Key << "sm_ortho_size" << YAML::Value << global.sm_ortho_size;
		yaml << YAML::EndMap;

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
				yaml << YAML::Key << "is_primary" << YAML::Value << camera.m_is_primary;
				yaml << YAML::EndMap;

				yaml << YAML::EndMap;
				break;
			}
		}

		yaml << YAML::Key << "entities" << YAML::Value;
		yaml << YAML::BeginSeq;

		for (auto const& entity : m_entities) {

			yaml << YAML::BeginMap;

			// TagComponent
			auto const& tag = entity->get_component<TagComponent>();
			yaml << YAML::Key << "name" << YAML::Value << tag.m_tag;
			yaml << YAML::Key << "id" << YAML::Value << tag.m_id;

			// TransformComponent
			auto const& transform = entity->get_component<TransformComponent>();
			yaml << YAML::Key << "transform" << YAML::Value;
			yaml << YAML::BeginMap;
			yaml << YAML::Key << "location" << YAML::Value << transform.m_location;
			yaml << YAML::Key << "rotation" << YAML::Value << transform.m_rotation;
			yaml << YAML::Key << "scale" << YAML::Value << transform.m_scale;
			if (transform.m_parent) {
				yaml << YAML::Key << "parent" << YAML::Value << transform_ptr_to_id[transform.m_parent];
			}
			else {
				yaml << YAML::Key << "parent" << YAML::Value << YAML::Null;
			}
			yaml << YAML::EndMap;

			// CameraComponent
			if (entity->has_component<CameraComponent>()) {
				auto const& camera = entity->get_component<CameraComponent>();
				yaml << YAML::Key << "camera" << YAML::Value;
				yaml << YAML::BeginMap;
				yaml << YAML::Key << "is_perspective" << YAML::Value << camera.m_is_perspective;
				yaml << YAML::Key << "intrinsic" << YAML::Value << camera.m_intrinsic.fov;
				yaml << YAML::Key << "near" << YAML::Value << camera.m_near;
				yaml << YAML::Key << "far" << YAML::Value << camera.m_far;
				yaml << YAML::Key << "aspect" << YAML::Value << camera.m_aspect;
				yaml << YAML::Key << "use_fixed_aspect" << YAML::Value << camera.m_use_fixed_aspect;
				yaml << YAML::Key << "is_primary" << YAML::Value << camera.m_is_primary;
				yaml << YAML::EndMap;
			}

			// StaticMeshComponent
			if (entity->has_component<StaticMeshComponent>()) {
				auto const& mesh = entity->get_component<StaticMeshComponent>();
				yaml << YAML::Key << "static_mesh" << YAML::Value;
				yaml << YAML::BeginMap;
				yaml << YAML::Key << "guid" << YAML::Value << mesh.m_mesh->m_meta.guid;
				yaml << YAML::EndMap;
			}

			// SkeletalMeshComponent
			if (entity->has_component<SkeletalMeshComponent>()) {
				auto const& mesh = entity->get_component<SkeletalMeshComponent>();
				yaml << YAML::Key << "skeletal_mesh" << YAML::Value;
				yaml << YAML::BeginMap;
				yaml << YAML::Key << "mesh" << YAML::Value << mesh.m_mesh->m_meta.guid;
				if (mesh.m_skeleton) {
					yaml << YAML::Key << "skeleton" << YAML::Value << mesh.m_skeleton->m_meta.guid;
				}
				else {
					yaml << YAML::Key << "skeleton" << YAML::Value << YAML::Null;
				}
				yaml << YAML::EndMap;
			}

			// SpriteComponent
			if (entity->has_component<SpriteComponent>()) {
				auto const& sprite = entity->get_component<SpriteComponent>();
				yaml << YAML::Key << "sprite" << YAML::Value;
				yaml << YAML::BeginMap;
				yaml << YAML::Key << "color" << YAML::Value << sprite.m_color;
				if (sprite.m_texture) {
					yaml << YAML::Key << "texture" << YAML::Value << sprite.m_texture->m_meta.guid;
				}
				else {
					yaml << YAML::Key << "texture" << YAML::Value << YAML::Null;
				}
				yaml << YAML::Key << "tiling_scale" << YAML::Value << sprite.m_tiling_scale;
				yaml << YAML::Key << "tiling_offset" << YAML::Value << sprite.m_tiling_offset;
				yaml << YAML::Key << "texcoords" << YAML::Value;
				yaml << YAML::BeginSeq;
				for (auto const& uv : sprite.m_texcoords) {
					yaml << uv;
				}
				yaml << YAML::EndSeq;
				yaml << YAML::EndMap;
			}

			// LightComponent
			if (entity->has_component<LightComponent>()) {
				auto const& light = entity->get_component<LightComponent>();
				yaml << YAML::Key << "light" << YAML::Value;
				yaml << YAML::BeginMap;
				yaml << YAML::Key << "type" << YAML::Value << light.m_type;
				yaml << YAML::Key << "color" << YAML::Value << light.m_color;
				yaml << YAML::Key << "intensity" << YAML::Value << light.m_intensity;
				yaml << YAML::Key << "range" << YAML::Value << light.m_range;
				yaml << YAML::Key << "inner_cone" << YAML::Value << light.m_inner_cone;
				yaml << YAML::Key << "outer_cone" << YAML::Value << light.m_outer_cone;
				yaml << YAML::Key << "cast_shadow" << YAML::Value << light.m_cast_shadow;
				yaml << YAML::EndMap;
			}

			yaml << YAML::EndMap;
		}
		yaml << YAML::EndSeq;
		yaml << YAML::EndMap;

		auto const& root = FileSystem::s_content_root;
		save_yaml((root / m_meta.path).concat(".yaml"), yaml);

		m_is_dirty = false;
	}

}
