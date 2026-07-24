#pragma once

#include "core/core.h"
#include <entt/entt.hpp>

namespace z1 {
	struct Scene;

	struct PhysicsSystem {
		static void update(Scene* scene, float dt);
	};

}
