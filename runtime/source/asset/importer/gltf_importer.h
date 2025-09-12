#pragma once

#include "core/io.h"
#include "asset/importer/importer.h"

namespace z1::io {

	struct GltfImporterSettings : ImporterSettings {

	};

	struct GltfImporter : Importer<GltfImporter, GltfImporterSettings> {

		static bool can_import(Filepath const& path) noexcept;

		static ImportResult import(GltfImporterSettings const& settings);

	};

}
