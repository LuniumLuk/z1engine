#pragma once

#include "core/core.h"
#include "render/shader.h"
#include "io/image_loader.h"
#include "io/mesh_loader.h"

namespace z1 {

	template<typename T>
	struct API AssetLoader {
		static std::shared_ptr<T> load(Filepath const&) {
			CORE_ASSERT(sizeof(T) == 0, "AssetLoader<T> not implemented for this asset type.");
		}
	};

	struct API AssetManager {
		AssetManager() {
			m_search_paths.push_back({});
			m_search_paths.push_back(g_runtime_context.m_file_system->m_engine_dir);
		}

		~AssetManager() {
			CORE_DEBUG("shutting down AssetManager ...");
		}

		template <typename T>
		std::shared_ptr<T> get(std::string path) {
			auto& map = storage<T>();

			auto it = map.find(path);
			if (it != map.end()) {
				return it->second;
			}

			auto asset = AssetLoader<T>::load(path);
			map[path] = asset;
			return asset;
		}

		std::vector<Filepath> const& get_search_paths() const {
			return m_search_paths;
		}

		void add_search_path(Filepath const& path) {
			m_search_paths.push_back(path);
		}

		Filepath resolve_path(Filepath const& path) const {
			// search in all configured search paths
			for (auto const& base_dir : get_search_paths()) {
				auto candidate = base_dir / path;
				if (std::filesystem::exists(candidate)) {
					return candidate;
				}
			}
			return {};
		}

	private:
		template<typename T>
		std::unordered_map<std::string, std::shared_ptr<T>>& storage() {
			auto type_hash = typeid(T).hash_code();

			// if storage for this type doesn't exist, create it
			auto it = m_storages.find(type_hash);
			if (it == m_storages.end()) {
				it = m_storages.emplace(
					type_hash,
					std::make_any<std::unordered_map<std::string, std::shared_ptr<T>>>()
				).first;
			}

			return *std::any_cast<std::unordered_map<std::string, std::shared_ptr<T>>>(&it->second);
		}

		std::vector<Filepath> m_search_paths;
		std::unordered_map<size_t, std::any> m_storages;
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
				return io::load_static_mesh(full_path);
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
