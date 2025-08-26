#pragma once

#include "core/core.h"
#include "core/io.h"
#include "core/asset_manager.h"

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
		Filepath root;
	};

	template <typename Settings>
	struct Importer {

		virtual ~Importer() = default;

		virtual bool can_import(Filepath const& path) noexcept = 0;

		virtual ImportResult import(Settings const& settings) = 0;

	};

}
