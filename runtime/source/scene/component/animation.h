#pragma once

#include "core/core.h"
#include "scene/component/base.h"
#include "animation/animation.h"
#include <vector>
#include <glm/glm.hpp>

namespace z1 {

	REFLECTED_STRUCT(AnimationComponent) {
		std::shared_ptr<Animation> animation_asset;
		float current_time = 0.0f;
		float speed = 1.0f;
		bool loop = true;
		bool playing = true;

		// Final bone matrices for rendering
		std::vector<glm::mat4> bone_matrices;

		AnimationComponent(std::shared_ptr<Animation> const& animation) noexcept
			: animation_asset(animation) {}

		DISABLE_COPY(AnimationComponent)

	};

}
