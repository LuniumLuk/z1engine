#pragma once

#include "core/core.h"
#include "glm/glm.hpp"

namespace z1 {

	enum class LightType : int {
		Directional = 0,
		Point = 1,
		Spot = 2
	};

	REFLECT_ENUM(LightType, Directional)
	REFLECT_ENUM(LightType, Point)
	REFLECT_ENUM(LightType, Spot)

	REFLECTED_STRUCT(LightComponent) {
		LightType m_type = LightType::Directional; // 0: Directional, 1: Point, 2: Spot
		glm::vec3 m_color = { 1.0f, 1.0f, 1.0f };
		float m_intensity = 1.0f;
		float m_range = 10.0f;
		float m_inner_cone = 20.0f; // degrees
		float m_outer_cone = 30.0f; // degrees
		bool m_cast_shadow = false;

		LightComponent() = default;
		LightComponent(LightType type, glm::vec3 const& color, float intensity)
			: m_type(type), m_color(color), m_intensity(intensity) {}

		DISABLE_COPY(LightComponent)

	};

	REFLECTED_FIELD(LightComponent, m_type, FF_Default)
	REFLECTED_FIELD(LightComponent, m_color, FF_Default, "[color]")
	REFLECTED_FIELD(LightComponent, m_intensity, FF_Default, "[drag]min=0.0")
	REFLECTED_FIELD(LightComponent, m_range, FF_Default, "[drag]min=0.0")
	REFLECTED_FIELD(LightComponent, m_inner_cone, FF_Default, "[slider]min=0.0,max=90.0")
	REFLECTED_FIELD(LightComponent, m_outer_cone, FF_Default, "[slider]min=0.0,max=90.0")
	REFLECTED_FIELD(LightComponent, m_cast_shadow, FF_Default)

}
