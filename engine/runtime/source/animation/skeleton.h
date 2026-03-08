#pragma once

#include "asset/asset.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace z1 {

	struct Bone {
		std::string name;
		int id = -1;
		int node_index = -1;
		int parent_id = -1;
		glm::mat4 offset_matrix = glm::mat4(1.0f); // Inverse bind pose
		glm::mat4 local_bind_transform = glm::mat4(1.0f);
	};

	struct API Skeleton : Asset<Skeleton> {
		std::vector<Bone> bones;

		static std::shared_ptr<Skeleton> load(Guid const& guid);
	};

}
