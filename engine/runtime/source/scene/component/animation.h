#pragma once

#include "core/core.h"
#include "scene/component/base.h"
#include "animation/animation.h"
#include <vector>
#include <glm/glm.hpp>

namespace z1 {

	REFLECTED_COMPONENT(AnimationComponent) {
		std::shared_ptr<Animation> animation_asset;
		float current_time = 0.0f;
		float speed = 1.0f;
		bool loop = true;
		bool playing = true;

		// Final bone matrices for rendering (GlobalTransform * InverseBindPose)
		std::vector<glm::mat4> bone_matrices;
		// Previous frame's bone matrices for TAA
		std::vector<glm::mat4> prev_bone_matrices;
		// Global bone transforms for debug drawing (GlobalTransform)
		std::vector<glm::mat4> global_bone_transforms;
		// Uniform buffer for bone matrices
		std::shared_ptr<UniformBuffer> bone_ubo;
		// Uniform buffer for previous frame's bone matrices
		std::shared_ptr<UniformBuffer> prev_bone_ubo;

		AnimationComponent(std::shared_ptr<Animation> const& animation = nullptr) noexcept
			: animation_asset(animation) {}

		DISABLE_COPY(AnimationComponent)

	};

	REFLECTED_FIELD(AnimationComponent, animation_asset, FF_Default, "[asset]type=animation")
	REFLECTED_FIELD(AnimationComponent, current_time,    FF_Default, "[input]min=0")
	REFLECTED_FIELD(AnimationComponent, speed,           FF_Default, "[input]min=0")
	REFLECTED_FIELD(AnimationComponent, loop,            FF_Default)
	REFLECTED_FIELD(AnimationComponent, playing,         FF_Default)

}
