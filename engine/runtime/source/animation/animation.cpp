#include "pch.h"
#include "animation/animation.h"
#include "asset/asset_manager.h"
#include "asset/binary_file.h"
#include "util/yaml.h"

namespace z1 {

	std::shared_ptr<Animation> Animation::load(Guid const& guid, AssetMeta const& meta, Filepath const& file) {
		PROFILE_FUNCTION();
		BinaryFile bf{};
		if (!bf.load(concat(file, ".bin"))) {
			CORE_ERROR("failed to load animation: {0}", file.generic_string());
			return nullptr;
		}

		YAML::Node node = YAML::Load(bf.get_yaml());
		auto anim = std::make_shared<Animation>();
		anim->m_meta = meta;

		anim->name = node["name"].as<std::string>();
		anim->duration = node["duration"].as<float>();
		anim->ticks_per_second = node["ticks_per_second"].as<float>();

		for (auto const& ch_node : node["channels"]) {
			AnimationChannel ch;
			ch.bone_name = ch_node["bone_name"].as<std::string>();
			ch.bone_id = ch_node["bone_id"].as<int>();

			size_t pos_count = ch_node["pos_count"].as<size_t>();
			size_t rot_count = ch_node["rot_count"].as<size_t>();
			size_t scl_count = ch_node["scl_count"].as<size_t>();

			size_t pos_offset = ch_node["pos_offset"].as<size_t>();
			size_t rot_offset = ch_node["rot_offset"].as<size_t>();
			size_t scl_offset = ch_node["scl_offset"].as<size_t>();

			if (pos_count > 0) {
				ch.position_keys.resize(pos_count);
				auto slice = bf.get_data_slice(pos_offset, pos_count * sizeof(PositionKeyframe));
				std::memcpy(ch.position_keys.data(), slice.ptr, slice.size);
			}
			if (rot_count > 0) {
				ch.rotation_keys.resize(rot_count);
				auto slice = bf.get_data_slice(rot_offset, rot_count * sizeof(RotationKeyframe));
				std::memcpy(ch.rotation_keys.data(), slice.ptr, slice.size);
			}
			if (scl_count > 0) {
				ch.scale_keys.resize(scl_count);
				auto slice = bf.get_data_slice(scl_offset, scl_count * sizeof(ScaleKeyframe));
				std::memcpy(ch.scale_keys.data(), slice.ptr, slice.size);
			}
			anim->channels.push_back(ch);
		}
		return anim;
	}
}
