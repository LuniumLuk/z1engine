#include "z1engine.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <imgui/imgui.h>
#include <imguizmo/ImGuizmo.h>

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
	glm::vec3 random_T = {
		Random::rfloat() * 10.0f,
		Random::rfloat() * 10.0f,
		Random::rfloat() * 10.0f,
		//0.0f, 0.0f, 0.0f,
	};
	glm::vec3 random_R = {
		Random::rfloat() * 180.0f,
		Random::rfloat() * 180.0f,
		Random::rfloat() * 180.0f,
	};
	glm::vec3 random_S = {
		0.1f + Random::rfloat() * 0.9f,
		0.1f + Random::rfloat() * 0.9f,
		0.1f + Random::rfloat() * 0.9f,
		//1.0f, 1.0f, 1.0f,
	};
	std::cout << "--------random input--------\n";
	std::cout << "location:\n";
	glm_utils::print(random_T);
	std::cout << "rotation:\n";
	glm_utils::print(random_R);
	std::cout << "scale:\n";
	glm_utils::print(random_S);

	{
		std::cout << "--------Test Composite from TRS--------\n";
		TransformComponent t{};
		t.m_location = random_T;
		t.m_rotation = random_R;
		t.m_scale = random_S;
		auto trans = t.get_local_transform();

		glm::mat4 T = glm::translate(glm::mat4(1.0f), t.m_location);
		glm::mat4 R = t.get_local_rotation();
		glm::mat4 S = glm::scale(glm::mat4(1.0f), t.m_scale);
		std::cout << "T:\n";
		glm_utils::print(T);
		std::cout << "R:\n";
		glm_utils::print(R);
		std::cout << "S:\n";
		glm_utils::print(S);

		std::cout << "--------ours:\n";
		glm_utils::print(trans);

		std::cout << "--------ImGuizmo:\n";
		ImGuizmo::RecomposeMatrixFromComponents(&random_T.x, &random_R.x, &random_S.x, &trans[0][0]);
		glm_utils::print(trans);

		std::cout << "--------Test Decomposite from Matrix--------\n";
		std::cout << "--------ours:\n";
		t.set_local_transform(trans);
		std::cout << "location:\n";
		glm_utils::print(t.m_location);
		std::cout << "rotation:\n";
		glm_utils::print(t.m_rotation);
		std::cout << "scale:\n";
		glm_utils::print(t.m_scale);
		std::cout << "--------ImGuizmo:\n";
		glm::vec3 translation{}, rotation{}, scale{};
		ImGuizmo::DecomposeMatrixToComponents(&trans[0][0], &translation.x, &rotation.x, &scale.x);
		std::cout << "location:\n";
		glm_utils::print(translation);
		std::cout << "rotation:\n";
		glm_utils::print(rotation);
		std::cout << "scale:\n";
		glm_utils::print(scale);
	}

	return 0;
}
