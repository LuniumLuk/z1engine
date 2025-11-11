#pragma once

#include "core/core.h"
#include "core/guid.h"
#include "render/shader.h"
#include "asset/asset.h"
#include "util/yaml.h"

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

	struct API AssetManager {
		AssetManager();
		~AssetManager();

		void scan_content();

		//void scan_assets(bool force_refresh = false);
		//void import_asset(Guid const& guid, bool force_refresh = false);
		void remove_asset(Guid const& guid);

		bool has_asset(Guid const& guid) const;
		AssetMeta get_meta(Guid const& guid) const;

		Guid get_guid_from_path(Filepath const& path) const {
			auto it = m_path_to_guid_mapping.find(path);
			if (it != m_path_to_guid_mapping.end()) {
				return it->second;
			}
			return {};
		}

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
				CORE_ERROR("failed to find asset with guid: {0}", guid);
				return nullptr;
			}

			auto asset = AssetLoader<T>::load(guid);

			map[guid] = asset;
			return asset;
		}

		template<typename T>
		void set(Guid const& guid, std::shared_ptr<T> const& asset) {
			auto& map = storage<T>();
			map[guid] = asset;
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

		static Filepath s_content_root;
		static std::vector<Filepath> s_shader_roots;
		std::unordered_map<size_t, std::any> m_storages;
		std::unordered_map<Guid, AssetMeta> m_asset_metas;
		std::unordered_set<Guid> m_guid_registry;
		std::unordered_map<Guid, Filepath> m_guid_to_file_mapping;
		std::unordered_map<Filepath, Guid> m_path_to_guid_mapping;
		std::unique_ptr<AssetNode> m_asset_tree_root;
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
