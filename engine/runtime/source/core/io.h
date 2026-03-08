#pragma once

#include "core/core.h"
#include <filesystem>
#include <string>
#include <fstream>
#include <iostream>

namespace z1 {

	using Filepath = std::filesystem::path;

	struct API FileSystem {
		std::string read_file(Filepath const& path) noexcept;
		bool write_file(Filepath const& path, void const* data, size_t size) noexcept;
		bool copy_file(Filepath const& src, Filepath const& dst, bool overwrite = true) noexcept;

#ifdef ENGINE_DIR
		Filepath m_engine_dir = ENGINE_DIR;
#else
		Filepath m_engine_dir = "../runtime";
#endif

		static Filepath s_content_root;
		static Filepath s_cache_root;
		static Filepath s_engine_root;

	};

	bool legalize_path(Filepath& path);

}
