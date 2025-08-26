#pragma once

#include "glm/glm.hpp"
#include "yaml-cpp/yaml.h"

namespace z1 {

	YAML::Emitter& operator<<(YAML::Emitter& out, glm::vec3 const& v) {
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, glm::vec4 const& v) {
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

}
