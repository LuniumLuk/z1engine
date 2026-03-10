#pragma once

#include "scene/scene.h"

namespace z1 {

	class ScriptSystem {
	public:
		static void update(Scene* scene, float dt);
		static void shutdown(Scene* scene);
	};

}
