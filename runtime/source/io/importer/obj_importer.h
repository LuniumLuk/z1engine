#pragma once

#include "core/io.h"
#include "io/importer/importer.h"
#include "render/mesh.h"

namespace z1::io {

	struct ObjImporterSettings : ImporterSettings {

	};

	struct ObjImporter : Importer<ObjImporter, ObjImporterSettings> {

		static bool can_import(Filepath const& path) noexcept;

		static ImportResult import(ObjImporterSettings const& settings);

	};

}
