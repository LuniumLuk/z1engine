#include "pch.h"
#include "io/exporter/scene_exporter.h"
#include "scene/entity.h"
#include "scene/component/base.h"
#include "scene/component/camera.h"
#include "scene/component/mesh.h"
#include "scene/component/sprite.h"
#include "util/yaml.h"

namespace z1::io {

	bool SceneExporter::export_scene(Filepath const& file, std::shared_ptr<Scene> const& scene) {

		// map transform pointer to entity ID for parent reference
		std::unordered_map<void*, uint32_t> transform_ptr_to_id;
		transform_ptr_to_id[nullptr] = INVALID_INDEX;
		for (auto const& entity : scene->m_entities) {
			transform_ptr_to_id[&entity->get_component<TransformComponent>()] = entity->get_component<TagComponent>().m_id;
		}

		YAML::Emitter yaml;

		yaml << YAML::BeginMap;

		// scene is saved as a meta file format
		yaml << YAML::Key << "guid" << YAML::Value << Guid::generate().value;
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
			yaml << YAML::Key << "parent" << YAML::Value;
			if (transform.m_parent) {
				yaml << transform_ptr_to_id[transform.m_parent];
			}
			else {
				yaml << YAML::Null;
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

			// MeshComponent
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
				yaml << YAML::Key << "texture" << YAML::Value << sprite.m_texture->m_guid.value;
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

		std::ofstream fout(file.string());
		fout << yaml.c_str();
		fout.close();

		return true;
	}

}
