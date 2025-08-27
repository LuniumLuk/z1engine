#include "pch.h"
#include "core/asset_manager.h"
#include "core/io.h"

#include "bakery.h"

namespace z1 {
	namespace fs = std::filesystem;

	bool AssetMetaData::save(Filepath const& p) const {
		YAML::Emitter emitter;

		emitter << YAML::BeginMap;
		emitter << YAML::Key << "guid" << YAML::Value << guid.value;
		emitter << YAML::Key << "type" << YAML::Value << type;
		emitter << YAML::Key << "path" << YAML::Value << path.generic_string();
		emitter << YAML::Key << "extra" << YAML::Value << extra;
		emitter << YAML::EndMap;

		try {
			fs::create_directories(p.parent_path());
			std::ofstream fout(p);
			fout << emitter.c_str();
			fout.close();
		}
		catch (std::exception const& e) {
			CORE_ERROR("failed to save to {}: {}", p.generic_string(), e.what());
			return false;
		}

		return true;
	}

	bool AssetMetaData::load(Filepath const& p) {
		try {
			YAML::Node node = YAML::LoadFile(p.string());

			guid = Guid::make(node["guid"].as<std::string>());
			type = node["type"].as<std::string>();
			path = node["path"].as<std::string>();
			extra = node["extra"];
		}
		catch (std::exception const& e) {
			CORE_ERROR("failed to load from {}: {}", p.generic_string(), e.what());
			return false;
		}

		return true;
	}

	AssetManager::AssetManager() {
		Filepath cwd = fs::current_path();
		CORE_INFO("current working directory: {0}", cwd.generic_string());

		CORE_INFO("content root: {0}", FileSystem::s_content_root.generic_string());
		CORE_INFO("engine shader root: {0}", FileSystem::s_engine_shader_root.generic_string());

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

				AssetMetaData meta;
				if (!meta.load(file)) {
					CORE_ERROR("failed to load meta file: {0}", file.generic_string());
					continue;
				}

				if (!register_asset(meta, root)) {
					continue;
				}
			}
		}

		{
			auto const& root = FileSystem::s_engine_shader_root;

			if (!fs::exists(root)) {
				CORE_WARN("missing engine shader root: {0}", root.generic_string());
			}

			for (auto& entry : fs::recursive_directory_iterator(root)) {
				if (!entry.is_regular_file())
					continue;

				auto const& file = entry.path();
				if (file.extension() != ".glsl")
					continue; // skip non-shader files

				Filepath path = fs::relative(file.parent_path() / file.stem(), root);

				AssetMetaData meta{};
				// using path as guid, since we currently don't have a better way to generate stable guids for files
				meta.guid = Guid::make(path.generic_string());
				meta.type = "shader";
				meta.path = path.generic_string();

				if (!register_asset(meta, root, ".glsl")) {
					continue;
				}
			}
		}
	}

	void AssetManager::remove_asset(Guid const& guid) {
		AssetMetaData const& meta = get_meta(guid);
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

	AssetMetaData AssetManager::get_meta(Guid const& guid) const {
		auto it = m_asset_metas.find(guid);
		if (it == m_asset_metas.end()) {
			CORE_ERROR("no asset found with guid: {0}", guid);
			return {};
		}
		return it->second;
	}

	bool AssetManager::register_asset(AssetMetaData const& meta, Filepath const& root, std::string const& ext) {
		Filepath file = root / meta.path;
		file += ext;

		if (!register_guid(meta.guid)) {
			CORE_ERROR("duplicate guid found: {0}, file: {1}", meta.guid, file.generic_string());
			return false;
		}

		m_asset_metas[meta.guid] = meta;
		m_guid_to_file_mapping[meta.guid] = file;
		m_path_to_guid_mapping[meta.path] = meta.guid;

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
