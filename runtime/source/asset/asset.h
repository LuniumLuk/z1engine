#pragma once

#include "core/core.h"
#include "core/io.h"
#include "core/guid.h"
#include "util/yaml.h"
#include "asset/asset_manager.h"

namespace z1 {

	struct AssetMeta {
		Guid guid;
		std::string type;
		Filepath path;
		// used by the editor to tell apart "engine" assets
		// from normal project assets. Not saved to disk.
		std::string root;
	};

	template <typename Derived>
	struct API Asset {

		AssetMeta m_meta;

		// an asset can come from:
		// 1. imported from raw source files (gltf, obj, png, jpg ...)
		// 2. created or modified on editor (material, scene ...)

		// general rules:
		// - every asset should be saved to disk (usually as a .yaml file)
		// - each asset has a uniformed meta data field (struct AssetMeta)
		// - the actual asset data can be stored in the same yaml file,
		//   or in a separate binary file if needed (e.g. mesh vertex/index buffer)

		// each asset type that derives from this base should implement:
		// - create(): create a new asset, register it to AssetManager, and save to disk
		// - load(): load an existing asset from disk using its guid
		// - save(): save the asset to disk
		//
		// the following wrappers are CRTP helper functions to call the derived class methods

		template<typename... Args>
		static std::shared_ptr<Derived> create(Args&&... args) {
			return Derived::create(std::forward<Args>(args)...);
		}

		static std::shared_ptr<Derived> load(Guid const& guid) {
			return Derived::load(guid);
		}

		void save() const {
			Derived::save();
		}

	};

	inline YAML::Emitter& operator<<(YAML::Emitter& out, AssetMeta const& v) {
		out << YAML::BeginMap;
		out << YAML::Key << "guid" << YAML::Value << v.guid.value;
		out << YAML::Key << "type" << YAML::Value << v.type;
		out << YAML::Key << "path" << YAML::Value << v.path.generic_string();
		out << YAML::EndMap;
		return out;
	}

}

namespace YAML {

	template<>
	struct convert<z1::AssetMeta> {
		static bool decode(Node const& node, z1::AssetMeta& rhs) {
			if (!node.IsMap()) {
				return false;
			}
			rhs.guid.value = node["guid"].as<std::string>();
			rhs.type = node["type"].as<std::string>();
			rhs.path = node["path"].as<std::string>();
			return true;
		}
	};

}
