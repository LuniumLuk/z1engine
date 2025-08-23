#pragma once

#include "core/core.h"
#include "core/io.h"
#include "core/asset_manager.h"

namespace z1::io {

	struct ImportResult {
		std::vector<AssetMetaData> assets;
		std::vector<Filepath> files;
		//std::vector<std::string> warnings;
		//std::vector<std::string> errors;
		bool success = false;
	};

	struct ImporterSettings {
		std::string name;
		Filepath source_path;
		Filepath target_path;
		Filepath cache_dir;
	};

}
