#pragma once

#include "glm/glm.hpp"
#include "yaml-cpp/yaml.h"

namespace YAML {

	template<>
	struct convert<glm::vec3> {
		static Node encode(glm::vec3 const& rhs) {
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(Node const& node, glm::vec3& rhs) {
			if (!node.IsSequence() || node.size() != 3) {
				return false;
			}
			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

} // namespace YAML

namespace z1 {

	inline YAML::Emitter& operator<<(YAML::Emitter& out, glm::vec3 const& v) {
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	inline YAML::Emitter& operator<<(YAML::Emitter& out, glm::vec4 const& v) {
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

}
