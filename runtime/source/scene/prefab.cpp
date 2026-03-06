#include "pch.h"
#include "scene/prefab.h"
#include "asset/asset_manager.h"

namespace z1 {

	std::shared_ptr<Prefab> Prefab::load(Guid const& guid) {
		auto prefab = std::make_shared<Prefab>();
		prefab->m_meta = g_runtime_context.m_asset_manager->get_meta(guid);
		
		auto file = g_runtime_context.m_asset_manager->get_file_from_guid(guid);
		try {
			// AssetManager typically returns file without extension for get_file_from_guid? 
			// No, it returns full path including extension if it was registered with it.
			// But Scene::load does `file.concat(".yaml")`.
			// AssetManager registration:
			// "file.extension() == ".yaml"" -> meta.root = "engine"
			// "file.extension() == ".glsl"" -> meta.root = "engine"
			// It seems .yaml extension is part of the file path in registration if it's a meta file?
			// But for Scene, it seems the file on disk is "name.scene.yaml"? Or "name.yaml"?
			// Scene::load: `yaml = YAML::LoadFile((file.concat(".yaml")).string());`
			// This suggests `file` from `get_file_from_guid` does NOT have `.yaml` at end?
			// Let's check `AssetManager::register_asset`.
			// `m_guid_to_file_mapping[meta.guid] = file;` where file is `root / meta.path`.
			// `meta.path` usually includes extension for assets like textures.
			// For Scenes? `Scene::create` sets `scene->m_meta.path = path;`
			// `path` usually has extension.
			// `Scene::save` does `save_yaml((root / m_meta.path).concat(".yaml"), yaml);`
			// This implies the file on disk has `.yaml` appended to `meta.path`.
			// So if `meta.path` is `myscene.scene`, file is `myscene.scene.yaml`.
			// So `get_file_from_guid` returns `myscene.scene`.
			// So `concat(".yaml")` is correct.
			// For Prefab, if we follow the same pattern: `myprefab.prefab` -> `myprefab.prefab.yaml`.
			
			auto yaml = YAML::LoadFile((file.concat(".yaml")).string());
			if (yaml["entities"]) {
				prefab->m_data = yaml["entities"];
			}
		}
		catch (std::exception const& e) {
			CORE_ERROR("failed to load prefab file: {0}, {1}", file.generic_string(), e.what());
		}

		return prefab;
	}

	std::vector<std::shared_ptr<Entity>> Prefab::instantiate(std::shared_ptr<Scene> target_scene) {
		if (!m_data || !m_data.IsSequence()) {
			return {};
		}
		return target_scene->create_entities_from_yaml(m_data);
	}

}
