#pragma once

#include "scene/scene.h"

#define MAX_BONES 1024

namespace z1 {

	struct AnimationSystem {
		static void update(Scene* scene, float dt);
	};

}
