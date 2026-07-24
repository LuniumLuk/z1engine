#pragma once

#include "core/core.h"
#include "scene/component/base.h"
#include "scene/component/collider.h"

namespace z1 {

	enum struct PhysicsMode : int {
		Static = 0,
		Kinematic = 1,
		Dynamic = 2,
	};

	REFLECT_ENUM(PhysicsMode, Static)
	REFLECT_ENUM(PhysicsMode, Kinematic)
	REFLECT_ENUM(PhysicsMode, Dynamic)

	REFLECTED_COMPONENT(PhysicsComponent) : Requires<ColliderComponent, TransformComponent> {
		PhysicsMode m_mode = PhysicsMode::Static;
		float m_mass = 1.0f;
		bool m_use_gravity = true;
		float m_linear_damping = 0.1f;

		PhysicsComponent() = default;
		PhysicsComponent(PhysicsComponent&&) = default;
		PhysicsComponent& operator=(PhysicsComponent&&) = default;

		DISABLE_COPY(PhysicsComponent)
	};

	REFLECTED_FIELD(PhysicsComponent, m_mode, FF_Default)
	REFLECTED_FIELD(PhysicsComponent, m_mass, FF_Default, "[drag]min=0.0")
	REFLECTED_FIELD(PhysicsComponent, m_use_gravity, FF_Default)
	REFLECTED_FIELD(PhysicsComponent, m_linear_damping, FF_Default, "[drag]min=0.0")

}
