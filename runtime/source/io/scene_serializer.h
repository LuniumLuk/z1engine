#pragma once

#include "core/io.h"
#include "scene/scene.h"

namespace z1::io {

	struct SceneSerializer {
		static bool serialize_scene(Filepath const& file, std::shared_ptr<Scene> const& scene);
		static std::shared_ptr<Scene> deserialize_scene(Filepath const& file);
	};

}
