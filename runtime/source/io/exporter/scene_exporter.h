#pragma once

#include "core/io.h"
#include "scene/scene.h"

namespace z1::io {

	struct SceneExporter {
		static bool export_scene(Filepath const& file, std::shared_ptr<Scene> const& scene);
	};

}
