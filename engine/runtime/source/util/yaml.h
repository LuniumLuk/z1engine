#pragma once

#include <filesystem>
#include "core/core.h"
#include "core/guid.h"
#include "core/io.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "yaml-cpp/yaml.h"

namespace YAML {

	template<>
	struct convert<glm::vec2> {
		static bool decode(Node const& node, glm::vec2& rhs) {
			if (!node.IsSequence() || node.size() != 2) {
				return false;
			}
			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3> {
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

	template<>
	struct convert<glm::vec4> {
		static bool decode(Node const& node, glm::vec4& rhs) {
			if (!node.IsSequence() || node.size() != 4) {
				return false;
			}
			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::mat4> {
		static bool decode(Node const& node, glm::mat4& rhs) {
			if (!node.IsSequence() || node.size() != 16) {
				return false;
			}
			float* ptr = glm::value_ptr(rhs);
			for (int i = 0; i < 16; ++i) {
				ptr[i] = node[i].as<float>();
			}
			return true;
		}
	};

	template<>
	struct convert<z1::Guid> {
		static bool decode(Node const& node, z1::Guid& rhs) {
			if (node.IsNull()) {
				rhs.value = "";
			}
			else {
				rhs.value = node.as<std::string>();
			}
			return true;
		}
	};

} // namespace YAML

namespace z1 {

	inline YAML::Emitter& operator<<(YAML::Emitter& out, glm::vec2 const& v) {
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

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

	inline YAML::Emitter& operator<<(YAML::Emitter& out, glm::mat4 const& v) {
		out << YAML::Flow;
		out << YAML::BeginSeq;
		float const* ptr = glm::value_ptr(v);
		for (int i = 0; i < 16; ++i) {
			out << ptr[i];
		}
		out << YAML::EndSeq;
		return out;
	}

	inline YAML::Emitter& operator<<(YAML::Emitter& out, Guid const& v) {
		if (v.is_valid()) {
			out << v.value;
		}
		else {
			out << YAML::Null;
		}
		return out;
	}

	inline bool save_yaml(Filepath const& file, YAML::Emitter& emitter) {
		try {
			std::filesystem::create_directories(file.parent_path());
			std::ofstream fout(file);
			fout << emitter.c_str();
			fout.close();
		}
		catch (std::exception const& e) {
			CORE_ERROR("failed to save to {}: {}", file.generic_string(), e.what());
			return false;
		}

		return true;
	}

}
