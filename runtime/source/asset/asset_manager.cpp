#include "pch.h"
#include "asset/asset_manager.h"
#include "core/io.h"
#include "util/yaml.h"

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
				else if (file.extension() == ".yaml") {
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
				else {
					continue;
				}

				if (!register_asset(meta, root)) {
					continue;
				}
			}
		}
	}

	bool AssetManager::remove_asset(Guid const& guid) {
		auto it = m_asset_metas.find(guid);
		if (it == m_asset_metas.end()) {
			return false;
		}

		auto meta = it->second;
		auto root = get_root_for_meta(meta);
		auto base = root / meta.path;

		auto remove_if_exists = [&](Filepath const& target) {
			if (fs::exists(target)) {
				fs::remove(target);
			}
		};

		remove_if_exists(base);
		{
			auto bin = base;
			bin += ".bin";
			remove_if_exists(bin);
		}
		{
			auto yaml = base;
			yaml += ".yaml";
			remove_if_exists(yaml);
		}
		{
			auto meta_file = base;
			meta_file += ".meta.yaml";
			remove_if_exists(meta_file);
		}

		remove_asset_node(meta);

		m_guid_registry.erase(guid);
		m_asset_metas.erase(it);
		m_guid_to_file_mapping.erase(guid);
		m_path_to_guid_mapping.erase(meta.path);

		return true;
	}

	bool AssetManager::move_asset(Guid const& guid, Filepath const& new_path) {
		auto it = m_asset_metas.find(guid);
		if (it == m_asset_metas.end()) {
			return false;
		}

		AssetMeta& meta = it->second;
		AssetMeta old_meta = meta;
		Filepath normalized = new_path.lexically_normal();

		if (normalized.empty() || normalized == "." || normalized == "..") {
			CORE_WARN("invalid target asset path: {0}", new_path.generic_string());
			return false;
		}

		if (normalized.is_absolute()) {
			CORE_WARN("asset target path must be relative: {0}", new_path.generic_string());
			return false;
		}

		if (normalized == meta.path) {
			return true;
		}

		if (m_path_to_guid_mapping.find(normalized) != m_path_to_guid_mapping.end()) {
			CORE_WARN("asset already exists at path: {0}", normalized.generic_string());
			return false;
		}

		auto root = get_root_for_meta(meta);
		auto old_base = root / old_meta.path;
		auto new_base = root / normalized;

		try {
			fs::create_directories(new_base.parent_path());
		}
		catch (std::exception const& e) {
			CORE_ERROR("failed to create directories for asset move: {0}", e.what());
			return false;
		}

		std::vector<std::pair<Filepath, Filepath>> moved;

		auto move_file = [&](Filepath const& source, Filepath const& destination) -> bool {
			if (!fs::exists(source)) {
				return true;
			}
			try {
				fs::rename(source, destination);
				moved.emplace_back(destination, source);
				return true;
			}
			catch (std::exception const& e) {
				CORE_ERROR("failed to move file from {0} to {1}: {2}", source.generic_string(), destination.generic_string(), e.what());
				return false;
			}
		};

		auto rollback = [&]() {
			for (auto it = moved.rbegin(); it != moved.rend(); ++it) {
				try {
					fs::rename(it->first, it->second);
				}
				catch (std::exception const& e) {
					CORE_ERROR("failed to rollback asset move from {0} to {1}: {2}", it->first.generic_string(), it->second.generic_string(), e.what());
				}
			}
		};

		auto move_with_suffix = [&](std::string const& suffix) -> bool {
			Filepath source = old_base;
			Filepath destination = new_base;
			if (!suffix.empty()) {
				source = Filepath(old_base.generic_string() + suffix);
				destination = Filepath(new_base.generic_string() + suffix);
			}
			return move_file(source, destination);
		};

		for (auto const& suffix : { std::string(""), std::string(".bin"), std::string(".yaml"), std::string(".meta.yaml") }) {
			if (!move_with_suffix(suffix)) {
				rollback();
				return false;
			}
		}

		meta.path = normalized;
		Filepath yaml_file = new_base;
		yaml_file.replace_extension(".yaml");

		YAML::Emitter emitter;
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "meta" << YAML::Value << meta;
		emitter << YAML::EndMap;

		if (!save_yaml(yaml_file, emitter)) {
			meta.path = old_meta.path;
			rollback();
			return false;
		}

		remove_asset_node(old_meta);

		m_path_to_guid_mapping.erase(old_meta.path);
		m_path_to_guid_mapping[normalized] = guid;
		m_guid_to_file_mapping[guid] = new_base;

		insert_asset_node(meta);

		return true;
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

		insert_asset_node(meta);
		return true;
	}

	bool AssetManager::register_guid(Guid const& guid) {
		auto [it, inserted] = m_guid_registry.insert(guid);

		if (!inserted) {
			return false;
		}

		return true;
	}

	void AssetManager::insert_asset_node(AssetMeta const& meta) {
		auto root = get_root_for_meta(meta);
		AssetNode* node = m_asset_tree_root.get();
		auto asset_name = meta.path.filename().string();
		auto editor_path = root / meta.path;

		for (auto const& part : editor_path) {
			auto name = part.string();
			bool is_folder = (name != asset_name);
			auto it = node->children.find(name);
			if (it == node->children.end()) {
				auto new_node = std::make_unique<AssetNode>();
				new_node->name = name;
				new_node->parent = node;
				node = (node->children[name] = std::move(new_node)).get();
			}
			else {
				node = it->second.get();
			}

			if (!is_folder) {
				node->meta = &m_asset_metas[meta.guid];
			}
		}
	}

	void AssetManager::remove_asset_node(AssetMeta const& meta) {
		auto root = get_root_for_meta(meta);
		auto editor_path = root / meta.path;

		std::vector<AssetNode*> nodes;
		nodes.push_back(m_asset_tree_root.get());

		for (auto const& part : editor_path) {
			auto name = part.string();
			auto it = nodes.back()->children.find(name);
			if (it == nodes.back()->children.end()) {
				return;
			}
			nodes.push_back(it->second.get());
		}

		if (nodes.size() <= 1) {
			return;
		}

		auto asset_node = nodes.back();
		auto parent = asset_node->parent;
		if (parent) {
			parent->children.erase(asset_node->name);
		}

		for (int i = static_cast<int>(nodes.size()) - 2; i > 0; --i) {
			auto current = nodes[i];
			if (current->children.empty() && current->meta == nullptr && current->parent) {
				current->parent->children.erase(current->name);
			}
			else {
				break;
			}
		}
	}

	Filepath AssetManager::get_root_for_meta(AssetMeta const& meta) const {
		if (meta.root == "engine") {
			return FileSystem::s_engine_root;
		}
		return FileSystem::s_content_root;
	}

}
