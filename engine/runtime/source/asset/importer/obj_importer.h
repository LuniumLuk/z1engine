#pragma once

#include "core/io.h"
#include "asset/importer/importer.h"
#include "asset/mesh.h"

namespace z1 {

	struct ObjImporterSettings : ImporterSettings {

	};

	struct ObjImporter : Importer<ObjImporter, ObjImporterSettings> {

		static bool can_import(Filepath const& path) noexcept;

		static ImportResult import(ObjImporterSettings const& settings);

	};

}
