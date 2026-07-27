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

		for (auto const& root : FileSystem::s_roots) {
			CORE_INFO("asset root [{0}]: {1} (priority: {2})",
				root.name.empty() ? "(default)" : root.name,
				root.path.generic_string(), root.priority);
		}

		scan_content();
	}

	AssetManager::~AssetManager() {
		CORE_DEBUG("shutting down AssetManager ...");
	}

	std::string AssetManager::build_internal_path(AssetMeta const& meta) const {
		if (meta.root.empty()) {
			return meta.path.string();
		}
		return ROOT_SEPARATOR + meta.root + "/" + meta.path.string();
	}

	void AssetManager::scan_content() {
		m_asset_metas.clear();
		m_guid_registry.clear();
		m_guid_to_file_mapping.clear();
		m_path_to_guid_mapping.clear();
		m_loaded_assets.clear();
		m_asset_tree_root = std::make_unique<AssetNode>();

		for (auto const& root_config : FileSystem::s_roots) {
			auto const& root = root_config.path;

			if (!fs::exists(root)) {
				if (root_config.name.empty()) {
					fs::create_directories(root);
				}
				else {
					CORE_WARN("missing asset root [{0}]: {1}",
						root_config.name, root.generic_string());
				}
				if (!fs::exists(root)) {
					continue;
				}
			}

			// Scan YAML meta files
			for (auto& entry : fs::recursive_directory_iterator(root)) {
				if (!entry.is_regular_file())
					continue;

				auto const& file = entry.path();
				if (file.extension() != ".yaml")
					continue;

				AssetMeta meta;
				try {
					YAML::Node node = YAML::LoadFile(file.string());
					meta = node["meta"].as<AssetMeta>();
					meta.root = root_config.name;
				}
				catch (std::exception const& e) {
					CORE_ERROR("failed to load meta file: {0}, {1}", file.generic_string(), e.what());
					continue;
				}


				if (!register_asset(meta, root)) {
					continue;
				}
			}

			// Scan Python scripts (only in default root and engine root)
			for (auto& entry : fs::recursive_directory_iterator(root)) {
				if (!entry.is_regular_file())
					continue;

				auto const& file = entry.path();
				if (file.extension() != ".py")
					continue;

				Filepath path = fs::relative(file, root);
				std::string path_str = path.generic_string();

				AssetMeta meta;
				meta.type = "script";
				meta.path = path_str;
				meta.root = root_config.name;

				meta.guid = Guid::make(path_str);

				// Check for duplicates
				std::string internal_path = build_internal_path(meta);
				if (m_path_to_guid_mapping.find(internal_path) != m_path_to_guid_mapping.end())
					continue;

				register_asset(meta, root);
			}

			// Scan GLSL shader files (engine root and any root with shaders)
			for (auto& entry : fs::recursive_directory_iterator(root)) {
				if (!entry.is_regular_file())
					continue;

				auto const& file = entry.path();
				if (file.extension() != ".glsl")
					continue;

				Filepath path = fs::relative(file.parent_path() / file.stem(), root);

				AssetMeta meta;
				meta.type = "shader";
				meta.path = path.generic_string();
				meta.root = root_config.name;

				meta.guid = Guid::make(path.generic_string());

				// Check for duplicates (YAML might already define this shader)
				std::string internal_path = build_internal_path(meta);
				if (m_path_to_guid_mapping.find(internal_path) != m_path_to_guid_mapping.end())
					continue;

				register_asset(meta, root);
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
		m_path_to_guid_mapping.erase(build_internal_path(meta));
		m_loaded_assets.erase(guid);

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

		{
			AssetMeta check_meta = meta;
			check_meta.path = normalized;
			std::string new_internal = build_internal_path(check_meta);
			if (m_path_to_guid_mapping.find(new_internal) != m_path_to_guid_mapping.end()) {
				CORE_WARN("asset already exists at path: {0}", normalized.generic_string());
				return false;
			}
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

		YAML::Node node = YAML::LoadFile((old_base.concat(".yaml")).string());
		node["meta"]["path"] = normalized.generic_string();

		for (auto const& suffix : {
			std::string(""),
			std::string(".bin"),
			std::string(".yaml"),
			std::string(".glsl"),
			std::string(".meta.yaml") }) {
			if (!move_with_suffix(suffix)) {
				rollback();
				return false;
			}
		}

		meta.path = normalized;
		Filepath yaml_file = new_base;
		yaml_file.replace_extension(".yaml");

		YAML::Emitter emitter;
		emitter << node;
		if (!save_yaml(yaml_file, emitter)) {
			meta.path = old_meta.path;
			rollback();
			return false;
		}

		remove_asset_node(old_meta);

		m_path_to_guid_mapping.erase(build_internal_path(old_meta));
		{
			AssetMeta new_meta = meta;
			new_meta.path = normalized;
			m_path_to_guid_mapping[build_internal_path(new_meta)] = guid;
		}
		m_guid_to_file_mapping[guid] = new_base;

		insert_asset_node(meta);

		if (auto* loaded_asset = get_loaded_asset(guid)) {
			loaded_asset->m_meta.path = normalized;
		}

		// TODO: update meta in instance if it's currently loaded in memory

		return true;
	}

	bool AssetManager::has_asset(Guid const& guid) const {
		return m_asset_metas.find(guid) != m_asset_metas.end();
	}

	bool AssetManager::has_path(Filepath const& path) const {
		return get_guid_from_path(path).is_valid();
	}

	Filepath AssetManager::legalize_import_path(Filepath const& path) const {
		Filepath ret = path;
		legalize_path(ret);
		return next_path_available(ret);
	}

	Filepath AssetManager::next_path_available(Filepath const& path) const {
		if (!has_path(path)) {
			return path;
		}

		int counter = 1;
		while (true) {
			Filepath new_path = path;
			new_path += "_" + std::to_string(counter);
			if (!has_path(new_path)) {
				return new_path;
			}
			++counter;
		}
	}

	AssetMeta AssetManager::get_meta(Guid const& guid) const {
		auto it = m_asset_metas.find(guid);
		if (it == m_asset_metas.end()) {
			CORE_ERROR("no asset found with guid: {0}", guid);
			return {};
		}
		return it->second;
	}

	Guid AssetManager::get_guid_from_path(Filepath const& path) const {
		// Resolve the path using Query mode (searches all roots for existing assets)
		auto [root_name, sub_path] = resolve_asset_path(path, PathResolveMode::Query);

		std::string internal_key = root_name.empty()
			? sub_path.string()
			: (ROOT_SEPARATOR + root_name + "/" + sub_path.string());

		auto it = m_path_to_guid_mapping.find(internal_key);
		if (it != m_path_to_guid_mapping.end()) {
			return it->second;
		}

		return {};
	}

	std::pair<std::string, Filepath> AssetManager::resolve_asset_path(
		Filepath const& path, PathResolveMode mode) const {

		std::string path_str = path.generic_string();

		// ROOT_SEPARATOR + rootname/ prefix explicitly targets a named root
		if (!path_str.empty() && path_str[0] == ROOT_SEPARATOR[0]) {
			auto slash_pos = path_str.find('/');
			if (slash_pos != std::string::npos) {
				std::string root_name = path_str.substr(1, slash_pos - 1);
				auto root_path = FileSystem::get_root_path(root_name);
				if (!root_path.empty()) {
					return { root_name, Filepath(path_str.substr(slash_pos + 1)) };
				}
				CORE_WARN("unknown root in asset path: {0}", path_str);
				return { "", path };
			}
		}

		// No ROOT_SEPARATOR prefix: behavior depends on mode
		if (mode == PathResolveMode::Query) {
			// Search through all roots by priority to find an existing asset
			auto ordered_roots = FileSystem::get_roots_ordered();
			for (auto* root_config : ordered_roots) {
				std::string internal_key;
				if (root_config->name.empty()) {
					internal_key = path_str;
				}
				else {
					internal_key = ROOT_SEPARATOR + root_config->name + "/" + path_str;
				}
				if (m_path_to_guid_mapping.find(internal_key) != m_path_to_guid_mapping.end()) {
					return { root_config->name, path };
				}
			}
		}

		// Create mode, or Query mode with no match found -> default root
		return { "", path };
	}

	Guid AssetManager::resolve_guid(std::string const& str) const {
		auto guid = get_guid_from_path(str);
		if (!guid.is_valid()) {
			guid = Guid::make(str);
		}
		return guid;
	}

	bool AssetManager::register_asset(AssetMeta const& meta, Filepath const& root) {
		Filepath file = root / meta.path;

		if (!register_guid(meta.guid)) {
			CORE_ERROR("duplicate guid found: {0}, file: {1}", meta.guid, file.generic_string());
			DEBUG_CHECK(false);
			return false;
		}

		m_asset_metas[meta.guid] = meta;
		m_guid_to_file_mapping[meta.guid] = file;
		m_path_to_guid_mapping[build_internal_path(meta)] = meta.guid;

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
		auto editor_path = root / meta.path;

		for (auto it = editor_path.begin(); it != editor_path.end(); ++it) {
			auto name = it->string();
			auto next = it;
			++next;
			bool is_last = (next == editor_path.end());

			auto found = node->children.find(name);
			if (found == node->children.end()) {
				auto new_node = std::make_unique<AssetNode>();
				new_node->name = name;
				new_node->parent = node;
				node = (node->children[name] = std::move(new_node)).get();
			}
			else {
				node = found->second.get();
			}

			if (is_last) {
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

	AssetBase* AssetManager::get_loaded_asset(Guid const& guid) const {
		auto it = m_loaded_assets.find(guid);
		if (it == m_loaded_assets.end()) {
			return nullptr;
		}
		return it->second;
	}

	void AssetManager::track_loaded_asset(Guid const& guid, AssetBase* asset) {
		if (asset) {
			m_loaded_assets[guid] = asset;
		}
		else {
			m_loaded_assets.erase(guid);
		}
	}

	Filepath AssetManager::get_root_for_meta(AssetMeta const& meta) const {
		return FileSystem::get_root_path(meta.root);
	}

}
