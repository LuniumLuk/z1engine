#pragma once

#include "scene/scene.h"

namespace z1 {

	struct ScriptSystem {
		static void update(Scene* scene, float dt);
		static void shutdown(Scene* scene);
	};

}
