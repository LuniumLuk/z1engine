#pragma once

#include "asset/asset.h"
#include "scene/scene.h"
#include "util/yaml.h"

namespace z1 {

	struct API Prefab : Asset<Prefab> {
		YAML::Node m_data; // Stores the 'entities' sequence from the yaml file

		static std::shared_ptr<Prefab> load(Guid const& guid);

		// Instantiate this prefab into the target scene
		// Returns the root entities of the instantiated prefab
		std::vector<std::shared_ptr<Entity>> instantiate(std::shared_ptr<Scene> target_scene);
	};

}
