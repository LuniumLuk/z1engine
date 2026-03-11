#include "pch.h"
#include "asset/importer/texture_importer.h"
#include "util/yaml.h"

#include "bakery.h"
#include "tinyexr/tinyexr.h"

namespace z1 {

	static bool file_is_ldr_image(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> exts = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".psd", ".gif", ".pic" };
		return std::find(exts.begin(), exts.end(), ext) != exts.end();
	}

	static bool file_is_hdr_image(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> exts = { ".exr" };
		return std::find(exts.begin(), exts.end(), ext) != exts.end();
	}

	bool TextureImporter::can_import(Filepath const& path) noexcept {
		return file_is_ldr_image(path) || file_is_hdr_image(path);
	}

	ImportResult TextureImporter::import(TextureImporterSettings const& settings) {
		ImportResult ret{};
		if (!can_import(settings.file)) {
			CORE_WARN("{0} is not a common texture file", settings.file.generic_string());
			return ret;
		}

		auto const& root = FileSystem::s_content_root;

		Filepath import_file = root / settings.path;
		import_file += ".bin";

		AssetMeta meta{};
		meta.guid = Guid::generate();
		meta.type = "texture2d";
		meta.path = settings.path;
		meta.extra["sampler_mode"] = (int)settings.sampler_mode;
		meta.extra["wrap_mode"] = (int)settings.wrap_mode;

		meta.path = g_runtime_context.m_asset_manager->legalize_import_path(meta.path);
		if (!g_runtime_context.m_asset_manager->register_asset(meta, root)) {
			return ret;
		}

		if (file_is_ldr_image(settings.file)) {
			bakery::compress_image(settings.file, import_file);
			meta.extra["hdr"] = false;
		}
		else if (file_is_hdr_image(settings.file)) {
			g_runtime_context.m_file_system->copy_file(settings.file, import_file);
			meta.extra["hdr"] = true;
		}

		YAML::Emitter yaml;

		yaml << YAML::BeginMap;
		yaml << YAML::Key << "meta" << YAML::Value << meta;
		yaml << YAML::EndMap;

		save_yaml(import_file.replace_extension(".yaml"), yaml);

		ret.assets.push_back(meta);
		ret.files.push_back(import_file);
		ret.success = true;

		return ret;
	}

}
