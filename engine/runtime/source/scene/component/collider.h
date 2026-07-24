#pragma once

#include "core/core.h"
#include "glm/glm.hpp"

namespace z1 {

	enum struct ColliderShape : int {
		Sphere = 0,
		Box = 1,
		Capsule = 2,
	};

	REFLECT_ENUM(ColliderShape, Sphere)
	REFLECT_ENUM(ColliderShape, Box)
	REFLECT_ENUM(ColliderShape, Capsule)

	REFLECTED_COMPONENT(ColliderComponent) {
		ColliderShape m_shape = ColliderShape::Sphere;
		// .x=radius (sphere/capsule), .xyz=half-extents (box), .y=half-height (capsule)
		glm::vec3 m_half_extents = { 0.5f, 0.5f, 0.5f };

		ColliderComponent() = default;
		ColliderComponent(ColliderComponent&&) = default;
		ColliderComponent& operator=(ColliderComponent&&) = default;

		DISABLE_COPY(ColliderComponent)
	};

	REFLECTED_FIELD(ColliderComponent, m_shape, FF_Default)
	REFLECTED_FIELD(ColliderComponent, m_half_extents, FF_Default, "[drag]min=0.01")

}
