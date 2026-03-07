#pragma once

#include "core/core.h"
#include "asset/mesh.h"
#include "animation/skeleton.h"
#include "animation/animation.h"

namespace z1 {

	REFLECTED_STRUCT(StaticMeshComponent) {
		std::shared_ptr<StaticMesh> m_mesh;

		StaticMeshComponent(std::shared_ptr<StaticMesh> const& mesh) noexcept
			: m_mesh(mesh) {}

		DISABLE_COPY(StaticMeshComponent)

	};

	REFLECTED_STRUCT(SkeletalMeshComponent) {
		std::shared_ptr<SkeletalMesh> m_mesh;
		std::shared_ptr<Skeleton> m_skeleton;

		SkeletalMeshComponent(
			std::shared_ptr<SkeletalMesh> const& mesh,
			std::shared_ptr<Skeleton> const& skeleton = nullptr) noexcept
			: m_mesh(mesh), m_skeleton(skeleton) {}

		DISABLE_COPY(SkeletalMeshComponent)

	};

}
