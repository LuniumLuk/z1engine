#pragma once

#include "asset/material.h"
#include <vector>

namespace z1 {

	struct StaticMeshComponent;
	struct SkeletalMeshComponent;
	struct AnimationComponent;

	// Culling primitives

	struct Plane {
		float a, b, c, d;
		float distance(glm::vec3 const& p) const {
			return a * p.x + b * p.y + c * p.z + d;
		}
		void normalize() {
			float len = std::sqrt(a * a + b * b + c * c);
			a /= len; b /= len; c /= len; d /= len;
		}
	};

	struct Frustum {
		Plane planes[6];
	};

	Frustum create_frustum(glm::mat4 const& m);
	bool is_aabb_in_frustum(Frustum const& f, glm::vec3 const& min, glm::vec3 const& max);
	bool is_mesh_visible(Frustum const& f, glm::mat4 const& transform, glm::vec3 const& min, glm::vec3 const& max);
	void get_skeletal_bounds(
		SkeletalMeshComponent const& mesh,
		AnimationComponent const& anim,
		glm::vec3& out_min,
		glm::vec3& out_max);

	// Halton low-discrepancy sequence

	float halton(int index, int base);

	// Visible draw-list structs

	struct VisibleStaticMesh {
		glm::mat4 transform;
		glm::mat4 prev_transform;
		StaticMeshComponent const* mesh;
	};

	struct VisibleSkeletalMesh {
		glm::mat4 transform;
		glm::mat4 prev_transform;
		SkeletalMeshComponent const* mesh;
		AnimationComponent const* anim;
	};

	struct VisibleDrawList {
		std::vector<VisibleStaticMesh> static_meshes;
		std::vector<VisibleSkeletalMesh> skeletal_meshes;
	};

}
