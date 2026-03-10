#pragma once

#include "scene/scene.h"

#define MAX_BONES 144

namespace z1 {

	struct AnimationSystem {
		static void update(Scene* scene, float dt);
	};

}
