#pragma once

#include "core/io.h"
#include "asset/importer/importer.h"
#include "render/image.h"

namespace z1 {

	struct ImageImporterSettings : ImporterSettings {
		SamplerMode sampler_mode = SamplerMode::Linear;
		WrapMode wrap_mode = WrapMode::Repeat;
	};

	struct ImageImporter : Importer<ImageImporter, ImageImporterSettings> {

		static bool can_import(Filepath const& path) noexcept;

		static ImportResult import(ImageImporterSettings const& settings);

	};

}
