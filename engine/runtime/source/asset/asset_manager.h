#pragma once

#include "core/core.h"
#include "core/guid.h"
#include "render/shader.h"
#include "asset/asset.h"
#include "util/yaml.h"
#include <type_traits>

namespace z1 {

	template<typename T>
	struct API AssetLoader {
		static std::shared_ptr<T> load(Guid const& guid) {
			return T::load(guid);
		}
	};

	struct AssetNode {
		std::string name;
		AssetMeta* meta = nullptr;
		AssetNode* parent = nullptr;
		std::unordered_map<std::string, std::unique_ptr<AssetNode>> children;

		bool is_root() const { return parent && parent->parent == nullptr; }
		bool is_folder() const { return meta == nullptr; }
		bool has_subfolder() const {
			for (auto const& [_, child] : children) {
				if (child->is_folder()) {
					return true;
				}
			}
			return false;
		}
	};

	enum class PathResolveMode {
		Query,   // search through all roots to find an existing asset
		Create,  // use the default root (for creating new assets)
	};

	struct API AssetManager {
		AssetManager();
		~AssetManager();

		void scan_content();

		//void scan_assets(bool force_refresh = false);
		//void import_asset(Guid const& guid, bool force_refresh = false);
		bool remove_asset(Guid const& guid);

		bool has_asset(Guid const& guid) const;
		bool has_path(Filepath const& path) const;
		Filepath legalize_import_path(Filepath const& path) const;
		Filepath next_path_available(Filepath const& path) const;
		AssetMeta get_meta(Guid const& guid) const;
		bool move_asset(Guid const& guid, Filepath const& new_path);

		Guid get_guid_from_path(Filepath const& path) const;
		Guid resolve_guid(std::string const& str) const;

		// Resolves a path to (root_name, sub_path).
		// "$engine/scene/demo" → ("engine", "scene/demo")   — explicit root
		// "my_asset" + Query  → searches all roots for existing asset
		// "my_asset" + Create → ("", "my_asset")            — default root
		std::pair<std::string, Filepath> resolve_asset_path(
			Filepath const& path, PathResolveMode mode = PathResolveMode::Create) const;

		template <typename T>
		std::shared_ptr<T> get(Filepath const& path) {
			auto const& guid = get_guid_from_path(path);
			if (!guid.is_valid()) {
				CORE_ERROR("failed to find asset with path: {0}", path);
				return nullptr;
			}

			return get<T>(guid);
		}

		template <typename T>
		std::shared_ptr<T> get(Guid const& guid) {
			auto& map = storage<T>();

			auto it = map.find(guid);
			if (it != map.end()) {
				return it->second;
			}

			if (m_guid_to_file_mapping.find(guid) == m_guid_to_file_mapping.end()) {
				DEBUG_CHECK(false);
				CORE_ERROR("failed to find asset with guid: {0}", guid);
				return nullptr;
			}

			auto asset = AssetLoader<T>::load(guid);

			map[guid] = asset;
			if constexpr (std::is_base_of_v<AssetBase, T>) {
				track_loaded_asset(guid, asset ? asset.get() : nullptr);
			}
			return asset;
		}

		template<typename T>
		void set(Guid const& guid, std::shared_ptr<T> const& asset) {
			auto& map = storage<T>();
			map[guid] = asset;
			if constexpr (std::is_base_of_v<AssetBase, T>) {
				track_loaded_asset(guid, asset ? asset.get() : nullptr);
			}
		}

		Filepath get_file_from_guid(Guid const& guid) const {
			if (m_guid_to_file_mapping.find(guid) != m_guid_to_file_mapping.end()) {
				return m_guid_to_file_mapping.at(guid);
			}
			return {};
		}

		Filepath resolve_path(Filepath const& path) const {
			// return if file already exists
			if (std::filesystem::exists(path)) {
				return path;
			}

			auto guid = get_guid_from_path(path);
			if (!guid.is_valid()) {
				return {};
			}

			auto file = get_file_from_guid(guid);
			if (!std::filesystem::exists(file)) {
				return {};
			}

			return file;
		}

		bool register_asset(AssetMeta const& meta, Filepath const& root);
		// check if guid is already registered, if not register it and return true
		bool register_guid(Guid const& guid);

		AssetBase* get_loaded_asset(Guid const& guid) const;

		std::vector<AssetMeta> get_all_metas() const {
			std::vector<AssetMeta> metas;
			metas.reserve(m_asset_metas.size());
			for (auto const& [guid, meta] : m_asset_metas) {
				metas.push_back(meta);
			}
			return metas;
		}

		std::vector<Filepath> find_references(Guid const& guid) const {
			std::vector<Filepath> refs;
			std::string guid_str = guid.value;
			for (auto const& [id, meta] : m_asset_metas) {
				if (meta.guid == guid) continue; // skip self

				// checks yaml files (meta.yaml, material.yaml, scene.yaml etc)
				// simplistic check: just read file and find string
				// This is slow but acceptable for a tool feature
				Filepath root = get_root_for_meta(meta);
				Filepath path = root / meta.path;

				// check .yaml
				Filepath yaml_path = path;
				yaml_path += ".yaml";
				if (std::filesystem::exists(yaml_path)) {
					std::ifstream file(yaml_path);
					std::string line;
					while (std::getline(file, line)) {
						if (line.find(guid_str) != std::string::npos) {
							refs.push_back(meta.path);
							break;
						}
					}
				}
			}
			return refs;
		}

		AssetNode* get_asset_tree_root() const { return m_asset_tree_root.get(); }

	private:
		template<typename T>
		std::unordered_map<Guid, std::shared_ptr<T>>& storage() {
			auto type_hash = typeid(T).hash_code();

			// if storage for this type doesn't exist, create it
			auto it = m_storages.find(type_hash);
			if (it == m_storages.end()) {
				it = m_storages.emplace(
					type_hash,
					std::make_any<std::unordered_map<Guid, std::shared_ptr<T>>>()
				).first;
			}

			return *std::any_cast<std::unordered_map<Guid, std::shared_ptr<T>>>(&it->second);
		}

		std::unordered_map<size_t, std::any> m_storages;
		std::unordered_map<Guid, AssetBase*> m_loaded_assets;
		std::unordered_map<Guid, AssetMeta> m_asset_metas;
		std::unordered_set<Guid> m_guid_registry;
		std::unordered_map<Guid, Filepath> m_guid_to_file_mapping;
		std::unordered_map<Filepath, Guid> m_path_to_guid_mapping;
		std::unique_ptr<AssetNode> m_asset_tree_root;

		void track_loaded_asset(Guid const& guid, AssetBase* asset);

		void insert_asset_node(AssetMeta const& meta);
		void remove_asset_node(AssetMeta const& meta);
		Filepath get_root_for_meta(AssetMeta const& meta) const;
		std::string build_internal_path(AssetMeta const& meta) const;
	};

	template<>
	struct API AssetLoader<Shader> {
		static std::shared_ptr<Shader> load(Guid const& guid) {
			auto file = g_runtime_context.m_asset_manager->get_file_from_guid(guid);
			if (!file.empty()) {
				auto shader = Shader::create(file.concat(".glsl"));
				shader->m_guid = guid;
				return shader;
			}
			CORE_ERROR("failed to load shader: {0}", guid.value);
			return nullptr;
		}
	};

}
