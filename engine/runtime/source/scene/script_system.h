#pragma once

#include "scene/scene.h"

namespace z1 {

	struct ScriptSystem {
		static void update(Scene* scene, float dt);
		static void shutdown(Scene* scene);
		static void set_blocked(bool blocked) { s_blocked = blocked; }
		static bool is_blocked() { return s_blocked; }

	private:
		static bool s_blocked;
	};

}
