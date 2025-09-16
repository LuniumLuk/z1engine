#include "pch.h"
#include "asset/asset_manager.h"
#include "core/io.h"

#include "bakery.h"

namespace z1 {
	namespace fs = std::filesystem;

	AssetManager::AssetManager() {
		Filepath cwd = fs::current_path();
		CORE_INFO("current working directory: {0}", cwd.generic_string());

		CORE_INFO("content root: {0}", FileSystem::s_content_root.generic_string());
		CORE_INFO("engine root: {0}", FileSystem::s_engine_root.generic_string());

		scan_content();
	}

	AssetManager::~AssetManager() {
		CORE_DEBUG("shutting down AssetManager ...");
	}

	void AssetManager::scan_content() {
		m_asset_metas.clear();
		m_guid_registry.clear();
		m_guid_to_file_mapping.clear();
		m_path_to_guid_mapping.clear();
		m_asset_tree_root = std::make_unique<AssetNode>();

		{
			auto const& root = FileSystem::s_content_root;

			if (!fs::exists(root)) {
				fs::create_directories(root);
			}

			for (auto& entry : fs::recursive_directory_iterator(root)) {
				if (!entry.is_regular_file())
					continue;

				auto const& file = entry.path();
				if (file.extension() != ".yaml")
					continue; // skip non-meta files

				AssetMeta meta;
				try {
					YAML::Node node = YAML::LoadFile(file.string());
					meta = node["meta"].as<AssetMeta>();
					meta.root = "content";
				}
				catch (std::exception const& e) {
					CORE_ERROR("failed to load meta file: {0}, {1}", file.generic_string(), e.what());
					continue;
				}

				if (!register_asset(meta, root)) {
					continue;
				}
			}
		}

		{
			auto const& root = FileSystem::s_engine_root;

			if (!fs::exists(root)) {
				CORE_WARN("missing engine root: {0}", root.generic_string());
			}

			for (auto& entry : fs::recursive_directory_iterator(root)) {
				if (!entry.is_regular_file())
					continue;

				auto const& file = entry.path();

				Filepath path = fs::relative(file.parent_path() / file.stem(), root);

				AssetMeta meta;
				if (file.extension() == ".glsl") {
					meta.guid = Guid::make(path.generic_string());
					meta.type = "shader";
					meta.path = path.generic_string();
					meta.root = "engine";
				}
				else {
					try {
						YAML::Node node = YAML::LoadFile(file.string());
						meta = node["meta"].as<AssetMeta>();
						meta.root = "engine";
					}
					catch (std::exception const& e) {
						CORE_ERROR("failed to load meta file: {0}, {1}", file.generic_string(), e.what());
						continue;
					}
				}

				if (!register_asset(meta, root)) {
					continue;
				}
			}
		}
	}

	void AssetManager::remove_asset(Guid const& guid) {
		AssetMeta const& meta = get_meta(guid);
		if (!meta.guid.is_valid()) {
			return;
		}

		Filepath file = resolve_path(meta.path);

		m_guid_registry.erase(guid);
		m_asset_metas.erase(m_asset_metas.find(guid));
		m_guid_to_file_mapping.erase(m_guid_to_file_mapping.find(guid));
		m_path_to_guid_mapping.erase(m_path_to_guid_mapping.find(meta.path));

		if (!file.empty()) {
			Filepath cached_meta_path = file;
			cached_meta_path += ".meta.yaml";
			fs::remove(file);
			fs::remove(cached_meta_path);
		}
	}

	bool AssetManager::has_asset(Guid const& guid) const {
		return m_asset_metas.find(guid) != m_asset_metas.end();
	}

	AssetMeta AssetManager::get_meta(Guid const& guid) const {
		auto it = m_asset_metas.find(guid);
		if (it == m_asset_metas.end()) {
			CORE_ERROR("no asset found with guid: {0}", guid);
			return {};
		}
		return it->second;
	}

	bool AssetManager::register_asset(AssetMeta const& meta, Filepath const& root) {
		Filepath file = root / meta.path;

		if (!register_guid(meta.guid)) {
			CORE_ERROR("duplicate guid found: {0}, file: {1}", meta.guid, file.generic_string());
			return false;
		}

		m_asset_metas[meta.guid] = meta;
		m_guid_to_file_mapping[meta.guid] = file;
		m_path_to_guid_mapping[meta.path] = meta.guid;

		AssetNode* node = m_asset_tree_root.get();
		auto asset_name = meta.path.filename();
		auto editor_path = meta.root / meta.path;
		for (auto const& part : editor_path) {
			auto name = part.string();
			bool is_folder = (name != asset_name.string());
			auto it = node->children.find(name);
			if (it == node->children.end()) {
				auto new_node = std::make_unique<AssetNode>();
				new_node->name = name;
				new_node->parent = node;
				if (!is_folder) {
					new_node->meta = &m_asset_metas[meta.guid];
				}
				node = (node->children[name] = std::move(new_node)).get();
			}
			else {
				node = it->second.get();
			}

			if (!is_folder) {
				node->meta = &m_asset_metas[meta.guid];
			}
		}

		return true;
	}

	bool AssetManager::register_guid(Guid const& guid) {
		auto [it, inserted] = m_guid_registry.insert(guid);

		if (!inserted) {
			return false;
		}

		return true;
	}

}
