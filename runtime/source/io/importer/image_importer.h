#pragma once

#include "core/io.h"
#include "io/importer/importer.h"
#include "render/image.h"

namespace z1::io {

	struct ImageImporterSettings : ImporterSettings {
		SamplerMode sampler_mode = SamplerMode::Linear;
		WrapMode wrap_mode = WrapMode::Repeat;
	};

	struct ImageImporter : Importer<ImageImporterSettings> {
		virtual ~ImageImporter() = default;
		bool can_import(Filepath const& path) noexcept override;
		ImportResult import(ImageImporterSettings const& settings) override;
	};

}
