#include "pch.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "scene/component/base.h"
#include "scene/component/camera.h"
#include "scene/component/mesh.h"
#include "scene/component/sprite.h"
#include "core/core.h"
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
		return entity;
	}

	std::shared_ptr<Entity> Scene::create_transient_entity(std::string const& name) {
		auto entity = create_entity_impl(name);
		m_transient_entities.push_back(entity);
		return entity;
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
	}

	std::shared_ptr<Entity> Scene::get_main_camera() const {
		return m_main_camera;
	}

	void Scene::on_update(float delta_time) {
		PROFILE_FUNCTION();
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

		g_runtime_context.m_renderer_forward->draw(shared_from_this(), m_main_framebuffer);
		g_runtime_context.m_renderer_2d->draw(shared_from_this(), m_main_framebuffer);
	}

	bool Scene::serialize(Filepath const& path, std::shared_ptr<Scene> const& scene) {

		// map transform pointer to entity ID for parent reference
		std::unordered_map<void*, uint32_t> transform_ptr_to_id;
		transform_ptr_to_id[nullptr] = INVALID_INDEX;
		for (auto const& entity : scene->m_entities) {
			transform_ptr_to_id[&entity->get_component<TransformComponent>()] = entity->get_component<TagComponent>().m_id;
		}

		YAML::Emitter yaml;

		yaml << YAML::BeginMap;

		auto const& root = FileSystem::s_content_root;

		AssetMeta meta{};
		meta.guid.value = path.generic_string();
		meta.type = "scene";
		meta.path = path;

		yaml << YAML::Key << "meta" << YAML::Value << meta;

		yaml << YAML::Key << "entities" << YAML::Value;
		yaml << YAML::BeginSeq;

		for (auto const& entity : scene->m_entities) {

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

			yaml << YAML::EndMap;
		}
		yaml << YAML::EndSeq;
		yaml << YAML::EndMap;

		return save_yaml((root / path).concat(".yaml"), yaml);
	}

	std::shared_ptr<Scene> Scene::deserialize(Filepath const& path) {
		auto scene = std::make_shared<Scene>();

		auto const& root = FileSystem::s_content_root;
		Filepath file = (root / path).concat(".yaml");

		std::ifstream stream(file.string());
		if (!stream.is_open()) {
			CORE_ERROR("Failed to open file: {}", file.generic_string());
			return scene;
		}

		YAML::Node yaml;
		try {
			yaml = YAML::Load(stream);
		}
		catch (YAML::ParserException& e) {
			CORE_ERROR("failed to parse yaml: {}", e.what());
			return scene;
		}

		if (!yaml["meta"] || !yaml["meta"]["type"] || yaml["meta"]["type"].as<std::string>() != "scene") {
			CORE_ERROR("not a scene file: {}", file.generic_string());
			return scene;
		}

		auto entities = yaml["entities"];
		if (!entities) {
			CORE_WARN("scene has no entities: {}", file.generic_string());
			return scene;
		}

		std::unordered_map<uint32_t, TransformComponent*> id_to_transform;
		std::vector<std::pair<TransformComponent*, uint32_t>> transform_parent_pairs;
		for (auto const& entity_yaml : entities) {
			// TagComponent
			auto entity = scene->create_entity(entity_yaml["name"].as<std::string>());
			id_to_transform[entity_yaml["id"].as<uint32_t>()] = &entity->get_component<TransformComponent>();

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
					if (scene->m_main_camera) {
						CORE_WARN("scene has multiple primary cameras, overriding previous primary camera");
					}
					scene->m_main_camera = entity;
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

			// SpriteComponent
			if (entity_yaml["sprite"]) {
				auto const& sprite_yaml = entity_yaml["sprite"];
				auto& sprite = entity->add_component<SpriteComponent>();
				sprite.m_color = sprite_yaml["color"].as<glm::vec4>();
				if (sprite_yaml["texture"] && !sprite_yaml["texture"].IsNull()) {
					sprite.m_texture = g_runtime_context.m_asset_manager->get<Texture2D>(Guid::make(sprite_yaml["texture"].as<std::string>()));
				}
				sprite.m_tiling_scale = sprite_yaml["tiling_scale"].as<glm::vec2>();
				sprite.m_tiling_offset = sprite_yaml["tiling_offset"].as<glm::vec2>();
				auto const& texcoords_yaml = sprite_yaml["texcoords"];
				for (size_t i = 0; i < sprite.m_texcoords.size(); ++i) {
					sprite.m_texcoords[i] = texcoords_yaml[i].as<glm::vec2>();
				}
			}
		}

		// resolve parent references
		for (auto const& [transform, parent_id] : transform_parent_pairs) {
			if (id_to_transform.find(parent_id) != id_to_transform.end()) {
				transform->m_parent = id_to_transform[parent_id];
			}
			else {
				CORE_WARN("failed to find parent with id {}", parent_id);
			}
		}

		return scene;
	}

}
