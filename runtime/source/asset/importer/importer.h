#pragma once

#include "core/core.h"
#include "core/io.h"
#include "asset/asset_manager.h"

namespace z1::io {

	struct ImportResult {
		std::vector<AssetMetaData> assets;
		std::vector<Filepath> files;
		std::vector<std::string> warnings;
		std::vector<std::string> errors;
		bool success = false;
	};

	struct ImporterSettings {
		Filepath file;
		Filepath path;
	};

	template <typename Derived, typename Settings>
	struct Importer {

		static bool can_import(Filepath const& path) noexcept {
			return Derived::can_import(path);
		}

		static ImportResult import(Settings const& settings) {
			return Derived::import(settings);
		}

	};

}
