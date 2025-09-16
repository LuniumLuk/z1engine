#pragma once

#include "core/io.h"
#include "asset/texture.h"
#include "asset/importer/importer.h"

namespace z1 {

	struct TextureImporterSettings : ImporterSettings {
		SamplerMode sampler_mode = SamplerMode::Linear;
		WrapMode wrap_mode = WrapMode::Repeat;
	};

	struct TextureImporter : Importer<TextureImporter, TextureImporterSettings> {

		static bool can_import(Filepath const& path) noexcept;

		static ImportResult import(TextureImporterSettings const& settings);

	};

}
