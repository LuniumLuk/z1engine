#include "pch.h"
#include "core/asset_manager.h"
#include "bakery.h"
#include "yaml-cpp/yaml.h"

namespace z1 {
	namespace fs = std::filesystem;

	AssetManager::AssetManager() {
		Filepath cwd = fs::current_path();
		CORE_INFO("current working directory: {0}", cwd.generic_string());

		m_search_roots.push_back("editor/asset");
		m_search_roots.push_back("runtime/asset");
		m_cache_dir = "cache";

		scan_assets();
	}

	AssetManager::~AssetManager() {
		CORE_DEBUG("shutting down AssetManager ...");
	}

	void AssetManager::scan_assets(bool force_refresh) {
		m_asset_metas.clear();
		m_guid_registry.clear();

		for (auto const& root : get_search_roots()) {
			if (!fs::exists(root)) {
				CORE_WARN("missing root path: {0}", root.generic_string());
				continue;
			}

			for (auto& entry : fs::recursive_directory_iterator(root)) {
				if (!entry.is_regular_file())
					continue;

				auto const& path = entry.path();
				if (path.extension() == ".yaml")
					continue; // skip meta files themselves

				// check if meta exists
				fs::path meta_path = path;
				meta_path += ".meta.yaml";

				AssetMetaData meta;
				bool has_meta = fs::exists(meta_path);
				if (force_refresh) {
					has_meta = false;
				}

				if (has_meta) {
					YAML::Node node = YAML::LoadFile(meta_path.string());

					meta.guid = Guid::make(node["guid"].as<std::string>());
					meta.type = node["type"].as<std::string>();
					meta.root = node["root"].as<std::string>();
					meta.path = node["path"].as<std::string>();
				}
				else {
					// --- guess type ---
					std::string type{};
					if (io::file_is_ldr_image(path) || io::file_is_hdr_image(path)) {
						type = "image";
					}
					else if (io::file_is_compressed_image(path)) {
						type = "image";
					}
					else if (io::file_is_obj_mesh(path)) {
						type = "static mesh";
					}
					else if (file_is_shader(path)) {
						type = "shader";
					}
					else {
						CORE_WARN("skip unknown asset: {0}", path.generic_string());
						continue;
					}

					// --- create new meta ---
					meta.guid = Guid::generate();
					meta.type = type;
					meta.root = root.generic_string();
					meta.path = fs::relative(path, root).generic_string();

					YAML::Emitter emitter;

					emitter << YAML::BeginMap;
					emitter << YAML::Key << "guid" << YAML::Value << meta.guid.value;
					emitter << YAML::Key << "type" << YAML::Value << meta.type;
					emitter << YAML::Key << "root" << YAML::Value << meta.root;
					emitter << YAML::Key << "path" << YAML::Value << meta.path;
					emitter << YAML::EndMap;

					CORE_INFO("create meta for asset: {0}", meta.path);

					fs::create_directories(meta_path.parent_path());
					std::ofstream fout(meta_path);
					fout << emitter.c_str();
					fout.close();
				}

				// --- register asset ---
				if (!register_guid(meta.guid)) {
					CORE_ERROR("duplicated guid detected: {0} for: {1}", meta.guid, meta.path);
					continue;
				}
				m_asset_metas[meta.guid] = meta;
				m_path_to_guid_mapping[meta.path] = meta.guid;

				// --- import asset ---
				import_asset(meta.guid, force_refresh);
			}
		}
	}

	void AssetManager::import_asset(Guid const& guid, bool force_refresh) {
		AssetMetaData const& meta = get_meta(guid);
		if (!meta.guid.is_valid()) {
			return;
		}

		// --- resolve the physical asset path ---
		Filepath root = meta.root;
		if (root.empty() || !fs::exists(root)) {
			CORE_ERROR("asset import failed, root not found for asset: {0}, root: {1}", meta.path, meta.root);
			return;
		}

		Filepath src_path = root / meta.path;
		if (!fs::exists(src_path)) {
			CORE_ERROR("asset import failed, source file not exists: {0}", src_path.generic_string());
			return;
		}

		// --- load meta.yaml ---
		Filepath meta_path = src_path;
		meta_path += ".meta.yaml";

		YAML::Node node = YAML::LoadFile(meta_path.string());
		std::string type = node["type"].as<std::string>();

		// --- process based on type ---
		Filepath cache_path;
		Filepath cache_meta_path;
		if (type == "image") {
			cache_path = m_cache_dir / (guid.value + ".bin");
			cache_meta_path = cache_path;
			cache_meta_path += ".meta.yaml";

			if (fs::exists(cache_path))

			fs::create_directories(cache_path.parent_path());

			if (!fs::exists(cache_path) || force_refresh) {
				if (io::file_is_compressed_image(src_path)) {
					copy_file(src_path, cache_path);
				}
				else if (io::file_is_ldr_image(src_path)) {
					bakery::compress_image(src_path, cache_path);
				}
				else if (io::file_is_hdr_image(src_path)) {
					copy_file(src_path, cache_path);
				}
			}
		}
		else if (type == "shader") {
			cache_path = m_cache_dir / meta.path;
			cache_meta_path = cache_path;
			cache_meta_path += ".meta.yaml";

			fs::create_directories(cache_path.parent_path());

			if (!fs::exists(cache_path) || force_refresh) {
				copy_file(src_path, cache_path);
			}
		}
		else if (type == "static mesh") {
			cache_path = m_cache_dir / meta.path;
			cache_meta_path = cache_path;
			cache_meta_path += ".meta.yaml";

			fs::create_directories(cache_path.parent_path());

			if (!fs::exists(cache_path) || force_refresh) {
				copy_file(src_path, cache_path);
			}

			if (src_path.extension() == ".obj") {
				Filepath mtl_path = meta.path;
				mtl_path.replace_extension(".mtl");
				Filepath src_mtl_path = root / mtl_path;
				if (fs::exists(src_mtl_path)) {
					Filepath cache_mtl_path = m_cache_dir / mtl_path;
					if (!fs::exists(cache_mtl_path) || force_refresh) {
						copy_file(src_mtl_path, cache_mtl_path);
					}
				}
			}
		}
		else {
			CORE_ERROR("asset import failed, unsupported type: {0}", type);
			return;
		}

		if (!fs::exists(cache_meta_path) || force_refresh) {
			CORE_INFO("import asset to cache: {0}", meta.path);
			copy_file(meta_path, cache_meta_path);
		}
		m_cached_files[meta.guid] = cache_path;
		m_path_to_guid_mapping[meta.path] = meta.guid;
	}

	void AssetManager::remove_asset(Guid const& guid, bool delete_source) {
		AssetMetaData const& meta = get_meta(guid);
		if (!meta.guid.is_valid()) {
			return;
		}

		m_guid_registry.erase(guid);
		m_asset_metas.erase(m_asset_metas.find(guid));
		m_path_to_guid_mapping.erase(m_path_to_guid_mapping.find(meta.path));

		auto it = m_cached_files.find(guid);
		if (it != m_cached_files.end()) {
			Filepath cached_meta_path = it->second;
			cached_meta_path += ".meta.yaml";
			fs::remove(it->second);
			fs::remove(cached_meta_path);
			m_cached_files.erase(it);
		}

		if (delete_source) {
			Filepath root = meta.root;
			Filepath src_path = root / meta.path;
			if (fs::exists(src_path)) {
				fs::remove(src_path);
			}

			Filepath meta_path = src_path;
			meta_path += ".meta.yaml";
			if (fs::exists(meta_path)) {
				fs::remove(meta_path);
			}
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

	bool AssetManager::register_guid(Guid const& guid) {
		auto [it, inserted] = m_guid_registry.insert(guid);

		if (!inserted) {
			return false;
		}

		return true;
	}

	bool AssetManager::copy_file(Filepath const& src, Filepath const& dst) const {
		try {
			fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
			return true;
		}
		catch (const fs::filesystem_error& e) {
			CORE_ERROR("failed to copy file from {0} to {1}, reason: {2}", src.generic_string(), dst.generic_string(), e.what());
			return false;
		}
		catch (const std::exception& e) {
			CORE_ERROR("failed to copy file from {0} to {1}, reason: {2}", src.generic_string(), dst.generic_string(), e.what());
			return false;
		}
	}

}
