#include "pch.h"
#include "asset/serializer/scene_serializer.h"
#include "asset/asset_manager.h"
#include "scene/entity.h"
#include "scene/component/base.h"
#include "scene/component/camera.h"
#include "scene/component/mesh.h"
#include "scene/component/sprite.h"
#include "util/yaml.h"

namespace z1::io {

	bool SceneSerializer::serialize(Filepath const& file, std::shared_ptr<Scene> const& scene) {

		// map transform pointer to entity ID for parent reference
		std::unordered_map<void*, uint32_t> transform_ptr_to_id;
		transform_ptr_to_id[nullptr] = INVALID_INDEX;
		for (auto const& entity : scene->m_entities) {
			transform_ptr_to_id[&entity->get_component<TransformComponent>()] = entity->get_component<TagComponent>().m_id;
		}

		YAML::Emitter yaml;

		yaml << YAML::BeginMap;

		// scene is saved as a meta file format
		yaml << YAML::Key << "guid" << YAML::Value << YAML::Null;
		yaml << YAML::Key << "type" << YAML::Value << "scene";
		yaml << YAML::Key << "path" << YAML::Value << YAML::Null;
		yaml << YAML::Key << "extra" << YAML::Value << YAML::Null;

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
				yaml << YAML::Key << "guid" << YAML::Value << mesh.m_mesh->m_guid.value;
				yaml << YAML::EndMap;
			}

			// SpriteComponent
			if (entity->has_component<SpriteComponent>()) {
				auto const& sprite = entity->get_component<SpriteComponent>();
				yaml << YAML::Key << "sprite" << YAML::Value;
				yaml << YAML::BeginMap;
				yaml << YAML::Key << "color" << YAML::Value << sprite.m_color;
				if (sprite.m_texture) {
					yaml << YAML::Key << "texture" << YAML::Value << sprite.m_texture->m_guid.value;
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

		std::filesystem::create_directories(file.parent_path());
		std::ofstream fout(file.string());
		fout << yaml.c_str();
		fout.close();

		return true;
	}

	std::shared_ptr<Scene> SceneSerializer::deserialize(Filepath const& file) {
		auto scene = std::make_shared<Scene>();

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

		if (!yaml["type"] || yaml["type"].as<std::string>() != "scene") {
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
					sprite.m_texture = g_runtime_context.m_asset_manager->get<Image2D>(Guid::make(sprite_yaml["texture"].as<std::string>()));
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
