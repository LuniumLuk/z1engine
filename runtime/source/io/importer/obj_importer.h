#pragma once

#include "core/io.h"
#include "io/importer/importer.h"
#include "render/mesh.h"

namespace z1::io {

	struct ObjImporterSettings : ImporterSettings {

	};

	struct ObjImporter : Importer<ObjImporterSettings> {
		virtual ~ObjImporter() = default;
		bool can_import(Filepath const& path) noexcept override;
		ImportResult import(ObjImporterSettings const& settings) override;
	};

}
