#pragma once

#include "core/core.h"
#include "asset/mesh.h"

namespace z1 {

	REFLECTED_STRUCT(StaticMeshComponent) {
		std::shared_ptr<StaticMesh> m_mesh;

		StaticMeshComponent(std::shared_ptr<StaticMesh> const& mesh) noexcept
			: m_mesh(mesh) {}
	};

}
