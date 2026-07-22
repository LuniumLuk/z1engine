#pragma once

#include "core/core.h"
#include "glm/glm.hpp"
#include "asset/texture.h"

namespace z1 {

	REFLECTED_COMPONENT(SkyLightComponent) {
		std::shared_ptr<Texture2D> m_texture;
		float m_rotation = 0.0f; // degrees around Y
		float m_intensity = 1.0f;
		float m_mip_level = 0.0f;

		SkyLightComponent() = default;

		DISABLE_COPY(SkyLightComponent)
	};

	REFLECTED_FIELD(SkyLightComponent, m_texture,   FF_Default)
	REFLECTED_FIELD(SkyLightComponent, m_rotation,  FF_Default, "[slider]min=0.0,max=360.0")
	REFLECTED_FIELD(SkyLightComponent, m_intensity, FF_Default, "[input]min=0.0")
	REFLECTED_FIELD(SkyLightComponent, m_mip_level, FF_Default, "[slider]min=0.0,max=10.0")

}
