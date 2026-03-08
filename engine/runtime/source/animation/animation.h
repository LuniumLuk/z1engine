#pragma once

#include "asset/asset.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace z1 {

	struct PositionKeyframe {
		float time;
		glm::vec3 value;
	};

	struct RotationKeyframe {
		float time;
		glm::quat value;
	};

	struct ScaleKeyframe {
		float time;
		glm::vec3 value;
	};

	struct AnimationChannel {
		std::string bone_name;
		int bone_id = -1;
		std::vector<PositionKeyframe> position_keys;
		std::vector<RotationKeyframe> rotation_keys;
		std::vector<ScaleKeyframe> scale_keys;
	};

	struct API Animation : Asset<Animation> {
		std::string name;
		float duration = 0.0f;
		float ticks_per_second = 0.0f;
		std::vector<AnimationChannel> channels;

		static std::shared_ptr<Animation> load(Guid const& guid);
	};

}
