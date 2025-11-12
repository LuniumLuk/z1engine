#pragma once

#include "glm/glm.hpp"

namespace z1 {

	struct alignas(16) GlobalConstants {
		glm::mat4 projview;
		glm::vec4 sun_direction;
		glm::vec4 sun_intensity;
		glm::vec4 cam_position;
	};

}