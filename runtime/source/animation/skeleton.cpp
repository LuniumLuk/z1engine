#include "pch.h"
#include "animation/skeleton.h"
#include "asset/asset_manager.h"
#include "asset/binary_file.h"
#include "util/yaml.h"

namespace z1 {

	std::shared_ptr<Skeleton> Skeleton::load(Guid const& guid) {
		PROFILE_FUNCTION();
		auto file = g_runtime_context.m_asset_manager->get_file_from_guid(guid);

		BinaryFile bf{};
		if (!bf.load(file.concat(".bin"))) {
			CORE_ERROR("failed to load skeleton: {0}", file.generic_string());
			return nullptr;
		}

		YAML::Node node = YAML::Load(bf.get_yaml());
		auto skeleton = std::make_shared<Skeleton>();
		skeleton->m_meta = g_runtime_context.m_asset_manager->get_meta(guid);

		if (node["bones"]) {
			for (auto const& n : node["bones"]) {
				Bone bone;
				bone.name = n["name"].as<std::string>();
				bone.id = n["id"].as<int>();
				bone.node_index = n["node_index"] ? n["node_index"].as<int>() : -1;
				bone.parent_id = n["parent_id"].as<int>();
				bone.offset_matrix = n["offset_matrix"].as<glm::mat4>();
				if (n["local_bind_transform"]) {
					bone.local_bind_transform = n["local_bind_transform"].as<glm::mat4>();
				}
				skeleton->bones.push_back(bone);
			}
		}
		return skeleton;
	}

}
