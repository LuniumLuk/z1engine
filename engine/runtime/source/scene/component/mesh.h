#pragma once

#include "core/core.h"
#include "asset/mesh.h"
#include "animation/skeleton.h"
#include "animation/animation.h"

namespace z1 {

	REFLECTED_COMPONENT(StaticMeshComponent) {
		std::shared_ptr<StaticMesh> m_mesh;

		StaticMeshComponent(std::shared_ptr<StaticMesh> const& mesh) noexcept
			: m_mesh(mesh) {}

		DISABLE_COPY(StaticMeshComponent)

	};

	REFLECTED_FIELD(StaticMeshComponent, m_mesh, FF_Default, "[asset]type=static_mesh")

	REFLECTED_COMPONENT(SkeletalMeshComponent) {
		std::shared_ptr<SkeletalMesh> m_mesh;
		std::shared_ptr<Skeleton> m_skeleton;

		SkeletalMeshComponent(
			std::shared_ptr<SkeletalMesh> const& mesh,
			std::shared_ptr<Skeleton> const& skeleton = nullptr) noexcept
			: m_mesh(mesh), m_skeleton(skeleton) {}

		DISABLE_COPY(SkeletalMeshComponent)

	};

	REFLECTED_FIELD(SkeletalMeshComponent, m_mesh,     FF_Default, "[asset]type=skeletal_mesh")
	REFLECTED_FIELD(SkeletalMeshComponent, m_skeleton, FF_Default, "[asset]type=skeleton")

}
