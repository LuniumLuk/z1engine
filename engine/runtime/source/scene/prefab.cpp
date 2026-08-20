#include "pch.h"
#include "scene/prefab.h"
#include "asset/asset_manager.h"

namespace z1 {

	std::shared_ptr<Prefab> Prefab::load(Guid const& guid, AssetMeta const& meta, Filepath const& file) {
		auto prefab = std::make_shared<Prefab>();
		prefab->m_meta = meta;

		auto yaml = YAML::LoadFile(concat(file, ".yaml").string());
		if (yaml["entities"]) {
			prefab->m_data = yaml["entities"];
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
