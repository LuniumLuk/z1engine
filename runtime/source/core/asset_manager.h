#pragma once

#include "core/core.h"
#include "core/guid.h"
#include "render/shader.h"
#include "io/image_loader.h"
#include "io/obj_loader.h"

namespace z1 {

	template <typename, typename = void>
	struct has_m_guid : std::false_type {};

	template <typename T>
	struct has_m_guid<T, std::void_t<decltype(std::declval<T>().m_guid)>> : std::true_type {};

	template<typename T>
	struct API AssetLoader {
		static std::shared_ptr<T> load(Filepath const&) {
			CORE_ASSERT(sizeof(T) == 0, "AssetLoader<T> not implemented for this asset type.");
		}
	};

	struct AssetMetaData {
		Guid guid;
		std::string type;
		std::string root;
		std::string path;
	};

	struct API AssetManager {
		AssetManager();
		~AssetManager();

		void scan_assets(bool force_refresh = false);
		void import_asset(Guid const& guid, bool force_refresh = false);
		void remove_asset(Guid const& guid, bool delete_source = true);

		bool has_asset(Guid const& guid) const;
		AssetMetaData get_meta(Guid const& guid) const;

		Guid get_guid_from_path(Filepath const& path) {
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

			if (m_cached_files.find(guid) == m_cached_files.end()) {
				CORE_ERROR("failed to find asset with guid: {0}", guid);
				return nullptr;
			}

			auto asset = AssetLoader<T>::load(m_cached_files[guid]);
			if constexpr (has_m_guid<T>::value) {
				asset->m_guid = guid;
			}

			map[guid] = asset;
			return asset;
		}

		std::vector<Filepath> const& get_search_roots() const {
			return m_search_roots;
		}

		void add_search_root(Filepath const& root) {
			m_search_roots.push_back(root);
		}

		Filepath resolve_path(Filepath const& path) const {
			// return if file already exists
			if (std::filesystem::exists(path)) {
				return path;
			}

			// search in all configured search paths
			for (auto const& root : get_search_roots()) {
				auto candidate = root / path;
				if (std::filesystem::exists(candidate)) {
					return candidate;
				}
			}
			return {};
		}

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

		bool register_guid(Guid const& guid);
		bool copy_file(Filepath const& src, Filepath const& dst) const;

		std::vector<Filepath> m_search_roots;
		Filepath m_cache_dir;
		std::unordered_map<size_t, std::any> m_storages;
		std::unordered_map<Guid, AssetMetaData> m_asset_metas;
		std::unordered_set<Guid> m_guid_registry;
		std::unordered_map<Guid, Filepath> m_cached_files;
		std::unordered_map<Filepath, Guid> m_path_to_guid_mapping;
	};

	template<>
	struct API AssetLoader<Image2D> {
		static std::shared_ptr<Image2D> load(Filepath const& path) {
			auto full_path = g_runtime_context.m_asset_manager->resolve_path(path);
			if (!full_path.empty()) {
				return io::load_image2d(full_path);
			}

			CORE_ERROR("failed to load Image2D asset: {0}", path);
			return nullptr;
		}
	};

	template<>
	struct API AssetLoader<StaticMesh> {
		static std::shared_ptr<StaticMesh> load(Filepath const& path) {
			auto full_path = g_runtime_context.m_asset_manager->resolve_path(path);
			if (!full_path.empty()) {
				return io::load_obj_mesh(full_path);
			}

			CORE_ERROR("failed to load StaticMesh asset: {0}", path);
			return nullptr;
		}
	};

	template<>
	struct API AssetLoader<Shader> {
		static std::shared_ptr<Shader> load(Filepath const& path) {
			auto full_path = g_runtime_context.m_asset_manager->resolve_path(path);
			if (!full_path.empty()) {
				return Shader::create(full_path);
			}

			CORE_ERROR("failed to load Shader asset: {0}", path);
			return nullptr;
		}
	};

}
