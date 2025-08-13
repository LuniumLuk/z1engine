#include "z1engine.h"
#include <iostream>

using namespace z1;

namespace glm_utils {
	static inline void print(const glm::vec2& v, const char* name = "") {
		if (name && *name) std::cout << name << ": ";
		std::cout << "(" << v.x << ", " << v.y << ")\n";
	}

	static inline void print(const glm::vec3& v, const char* name = "") {
		if (name && *name) std::cout << name << ": ";
		std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ")\n";
	}

	static inline void print(const glm::vec4& v, const char* name = "") {
		if (name && *name) std::cout << name << ": ";
		std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")\n";
	}

	static inline void print(const glm::mat3& m, const char* name = "") {
		if (name && *name) std::cout << name << ":\n";
		for (int row = 0; row < 3; ++row) {
			std::cout << "| ";
			for (int col = 0; col < 3; ++col)
				std::cout << std::setw(8) << m[col][row] << " ";
			std::cout << "|\n";
		}
	}

	static inline void print(const glm::mat4& m, const char* name = "") {
		if (name && *name) std::cout << name << ":\n";
		for (int row = 0; row < 4; ++row) {
			std::cout << "| ";
			for (int col = 0; col < 4; ++col)
				std::cout << std::setw(8) << m[col][row] << " ";
			std::cout << "|\n";
		}
	}
}

int main() {

	std::cout << "Hello World!\n";
	TransformComponent t{};
	t.m_location = {
		Random::rfloat(),
		Random::rfloat(),
		Random::rfloat(),
	};
	t.m_rotation = {
		Random::rfloat(),
		Random::rfloat(),
		Random::rfloat(),
	};
	t.m_scale = {
		Random::rfloat(),
		Random::rfloat(),
		Random::rfloat(),
	};
	std::cout << "--------before set_local_transform--------\n";
	std::cout << "location:\n";
	glm_utils::print(t.m_location);
	std::cout << "rotation:\n";
	glm_utils::print(t.m_rotation);
	std::cout << "scale:\n";
	glm_utils::print(t.m_scale);
	std::cout << "local transform:\n";
	glm_utils::print(t.get_local_transform());

	auto trans = t.get_local_transform();
	t.set_local_transform(trans);

	std::cout << "--------after set_local_transform--------\n";
	std::cout << "location:\n";
	glm_utils::print(t.m_location);
	std::cout << "rotation:\n";
	glm_utils::print(t.m_rotation);
	std::cout << "scale:\n";
	glm_utils::print(t.m_scale);
	std::cout << "local transform:\n";
	glm_utils::print(t.get_local_transform());
}
