#pragma once

#include "core/io.h"
#include "io/importer/importer.h"

namespace z1::io {

	struct GltfImporterSettings : ImporterSettings {

	};

	struct API GltfImporter {

		static bool can_import(Filepath const& path);

		static ImportResult import(GltfImporterSettings const& settings);

	};

}
