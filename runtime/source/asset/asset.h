#pragma once

#include "core/core.h"
#include "core/io.h"
#include "core/guid.h"
#include "util/yaml.h"

namespace z1 {

	struct AssetMeta {
		Guid guid;
		std::string type;
		Filepath path;
		// used by the editor to tell apart "engine" assets
		// from normal project assets. Not saved to disk.
		std::string root;

		YAML::Node extra; // extra fields for each asset type

		std::string name() const {
			return path.filename().string();
		}
	};

	template <typename...>
	using void_t = void; // pre-C++17 compatibility

	// detect create()
	template <typename T, typename = void, typename... Args>
	struct has_create : std::false_type {};

	template <typename T, typename... Args>
	struct has_create<T, void_t<decltype(T::create(std::declval<Args>()...))>, Args...>
		: std::true_type {
	};

	// detect load()
	template <typename T, typename = void>
	struct has_load : std::false_type {};

	template <typename T>
	struct has_load<T, void_t<decltype(T::load(std::declval<Guid const&>()))>>
		: std::true_type {
	};

	// detect save()
	template <typename T, typename = void>
	struct has_save : std::false_type {};

	template <typename T>
	struct has_save<T, void_t<decltype(std::declval<T const*>()->save())>>
		: std::true_type {
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

		// enable only if Derived::create exists
		template<typename... Args, typename = std::enable_if_t<has_create<Derived, void, Args...>::value>>
		static std::shared_ptr<Derived> create(Args&&... args) {
			return Derived::create(std::forward<Args>(args)...);
		}

		// enable only if Derived::load exists
		template<typename D = Derived, typename = std::enable_if_t<has_load<D>::value>>
		static std::shared_ptr<Derived> load(Guid const& guid) {
			return Derived::load(guid);
		}

		// enable only if Derived::save exists
		template<typename D = Derived, typename = std::enable_if_t<has_save<D>::value>>
		void save() const {
			static_cast<Derived const*>(this)->save();
		}

	};

	inline YAML::Emitter& operator<<(YAML::Emitter& out, AssetMeta const& v) {
		out << YAML::BeginMap;
		out << YAML::Key << "guid" << YAML::Value << v.guid;
		out << YAML::Key << "type" << YAML::Value << v.type;
		out << YAML::Key << "path" << YAML::Value << v.path.generic_string();
		if (v.extra.IsNull()) {
			out << YAML::Key << "extra" << YAML::Value << YAML::Null;
		}
		else {
			out << YAML::Key << "extra" << YAML::Value << v.extra;
		}
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
			rhs.extra = node["extra"];
			return true;
		}
	};

}
