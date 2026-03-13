#include "pch.h"
#include "render/render_utils.h"
#include "scene/component/mesh.h"
#include "scene/component/animation.h"

namespace z1 {

	float halton(int index, int base) {
		float f = 1.0f;
		float r = 0.0f;
		while (index > 0)
		{
			f /= base;
			r += f * (index % base);
			index /= base;
		}
		return r;
	}

	Frustum create_frustum(glm::mat4 const& m) {
		Frustum f;
		// Left
		f.planes[0].a = m[0][3] + m[0][0];
		f.planes[0].b = m[1][3] + m[1][0];
		f.planes[0].c = m[2][3] + m[2][0];
		f.planes[0].d = m[3][3] + m[3][0];
		// Right
		f.planes[1].a = m[0][3] - m[0][0];
		f.planes[1].b = m[1][3] - m[1][0];
		f.planes[1].c = m[2][3] - m[2][0];
		f.planes[1].d = m[3][3] - m[3][0];
		// Bottom
		f.planes[2].a = m[0][3] + m[0][1];
		f.planes[2].b = m[1][3] + m[1][1];
		f.planes[2].c = m[2][3] + m[2][1];
		f.planes[2].d = m[3][3] + m[3][1];
		// Top
		f.planes[3].a = m[0][3] - m[0][1];
		f.planes[3].b = m[1][3] - m[1][1];
		f.planes[3].c = m[2][3] - m[2][1];
		f.planes[3].d = m[3][3] - m[3][1];
		// Near
		f.planes[4].a = m[0][3] + m[0][2];
		f.planes[4].b = m[1][3] + m[1][2];
		f.planes[4].c = m[2][3] + m[2][2];
		f.planes[4].d = m[3][3] + m[3][2];
		// Far
		f.planes[5].a = m[0][3] - m[0][2];
		f.planes[5].b = m[1][3] - m[1][2];
		f.planes[5].c = m[2][3] - m[2][2];
		f.planes[5].d = m[3][3] - m[3][2];

		for (int i = 0; i < 6; ++i) f.planes[i].normalize();
		return f;
	}

	bool is_aabb_in_frustum(Frustum const& f, glm::vec3 const& min, glm::vec3 const& max) {
		for (int i = 0; i < 6; ++i) {
			glm::vec3 p = min;
			if (f.planes[i].a >= 0) p.x = max.x;
			if (f.planes[i].b >= 0) p.y = max.y;
			if (f.planes[i].c >= 0) p.z = max.z;

			if (f.planes[i].distance(p) < 0) return false;
		}
		return true;
	}

	bool is_mesh_visible(Frustum const& f, glm::mat4 const& transform, glm::vec3 const& min, glm::vec3 const& max) {
		glm::vec3 corners[8] = {
			{min.x, min.y, min.z}, {min.x, min.y, max.z},
			{min.x, max.y, min.z}, {min.x, max.y, max.z},
			{max.x, min.y, min.z}, {max.x, min.y, max.z},
			{max.x, max.y, min.z}, {max.x, max.y, max.z}
		};

		glm::vec3 world_min(FLT_MAX), world_max(-FLT_MAX);
		for (int i = 0; i < 8; ++i) {
			glm::vec3 p = glm::vec3(transform * glm::vec4(corners[i], 1.0f));
			world_min = glm::min(world_min, p);
			world_max = glm::max(world_max, p);
		}
		return is_aabb_in_frustum(f, world_min, world_max);
	}

	void get_skeletal_bounds(
		SkeletalMeshComponent const& mesh,
		AnimationComponent const& anim,
		glm::vec3& out_min,
		glm::vec3& out_max)
	{
		if (anim.global_bone_transforms.empty()) {
			out_min = mesh.m_mesh->m_bound_min;
			out_max = mesh.m_mesh->m_bound_max;
			return;
		}

		glm::vec3 min_bones(FLT_MAX);
		glm::vec3 max_bones(-FLT_MAX);

		for (auto const& m : anim.global_bone_transforms) {
			glm::vec3 pos = glm::vec3(m[3]); // Translation
			min_bones = glm::min(min_bones, pos);
			max_bones = glm::max(max_bones, pos);
		}

		// Calculate static size for padding
		glm::vec3 static_size = mesh.m_mesh->m_bound_max - mesh.m_mesh->m_bound_min;
		// Use the full length of the diagonal as padding to be conservative and avoid early culling
		float padding = glm::length(static_size);

		out_min = min_bones - glm::vec3(padding);
		out_max = max_bones + glm::vec3(padding);
	}

}
