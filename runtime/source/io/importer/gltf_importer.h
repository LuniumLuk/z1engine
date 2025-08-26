#pragma once

#include "core/io.h"
#include "io/importer/importer.h"

namespace z1::io {

	struct GltfImporterSettings : ImporterSettings {

	};

	struct GltfImporter : Importer<GltfImporterSettings> {
		virtual ~GltfImporter() = default;
		bool can_import(Filepath const& path) noexcept override;
		ImportResult import(GltfImporterSettings const& settings) override;
	};

}
