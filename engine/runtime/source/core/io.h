#pragma once

#include "core/core.h"
#include <filesystem>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <vector>
#include <algorithm>

namespace z1 {

	using Filepath = std::filesystem::path;

	struct RootConfig {
		std::string name;     // empty string = default root
		Filepath path;        // filesystem path (e.g., "content", "engine/content")
		int priority = 0;     // lower = searched first for unprefixed lookups
	};

	struct API FileSystem {
		std::string read_file(Filepath const& path) noexcept;
		bool write_file(Filepath const& path, void const* data, size_t size) noexcept;
		bool copy_file(Filepath const& src, Filepath const& dst, bool overwrite = true) noexcept;

#ifdef ENGINE_DIR
		Filepath m_engine_dir = ENGINE_DIR;
#else
		Filepath m_engine_dir = "../runtime";
#endif

		static std::vector<RootConfig> s_roots;
		static Filepath s_cache_root;

		static Filepath get_root_path(std::string const& name);
		static std::vector<RootConfig const*> get_roots_ordered();

	};

	bool legalize_path(Filepath& path);

	struct API Args {

		void parse(int argc, char* argv[]);

		template <typename T>
		T get(std::string const& key, T const& default_value = T()) const {
			auto it = m_args.find(key);
			if (it != m_args.end()) {
				std::istringstream iss(it->second);
				T value = default_value;
				if (iss >> value) {
					return value;
				}
			}
			return default_value;
		}

		template <>
		bool get<bool>(std::string const& key, bool const& default_value) const {
			auto it = m_args.find(key);
			if (it != m_args.end()) {
				std::string value = it->second;
				std::transform(value.begin(), value.end(), value.begin(), ::tolower);
				if (value == "true" || value == "1" || value == "") {
					return true;
				}
				else if (value == "false" || value == "0") {
					return false;
				}
			}
			return default_value;
		}

	private:
		std::unordered_map<std::string, std::string> m_args;

	};

	extern Args g_args;

}
