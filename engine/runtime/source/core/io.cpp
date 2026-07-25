#include "pch.h"
#include "core/io.h"

namespace z1 {

	Args g_args;

	std::vector<RootConfig> FileSystem::s_roots = {
		{"",       "content",        0},
		{"engine", "engine/content", 10},
	};
	Filepath FileSystem::s_cache_root = "cache";

	Filepath FileSystem::get_root_path(std::string const& name) {
		for (auto& root : s_roots) {
			if (root.name == name) {
				return root.path;
			}
		}
		return {};
	}

	std::vector<RootConfig const*> FileSystem::get_roots_ordered() {
		std::vector<RootConfig const*> result;
		for (auto& root : s_roots) {
			result.push_back(&root);
		}
		std::sort(result.begin(), result.end(), [](RootConfig const* a, RootConfig const* b) {
			return a->priority < b->priority;
		});
		return result;
	}

	std::string FileSystem::read_file(Filepath const& path) noexcept {
		PROFILE_FUNCTION();
		std::ifstream in(path.string(), std::ios::in | std::ios::binary);
		std::string ret;
		if (in.is_open()) {
			in.seekg(0, std::ios::end);
			ret.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&ret[0], ret.size());
			in.close();
		}
		else {
			CORE_ERROR("failed to read file: {0}", path);
		}
		return ret;
	}

	bool FileSystem::write_file(Filepath const& path, void const* data, size_t size) noexcept {
		PROFILE_FUNCTION();
		std::ofstream out(path.string(), std::ifstream::out | std::ios::binary);
		if (out.is_open()) {
			out.write((char*)data, size);
			out.close();
			return true;
		}
		else {
			CORE_ERROR("failed to write file: {0}", path);
		}
		return false;
	}

	bool FileSystem::copy_file(Filepath const& src, Filepath const& dst, bool overwrite) noexcept {
		namespace fs = std::filesystem;
		try {
			fs::copy_file(src, dst, overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::skip_existing);
			return true;
		}
		catch (const fs::filesystem_error& e) {
			CORE_ERROR("failed to copy file from {0} to {1}, reason: {2}", src.generic_string(), dst.generic_string(), e.what());
			return false;
		}
		catch (const std::exception& e) {
			CORE_ERROR("failed to copy file from {0} to {1}, reason: {2}", src.generic_string(), dst.generic_string(), e.what());
			return false;
		}
	}

	bool legalize_path(Filepath& path) {
		if (path.empty())
			return false;

		// 1. Reserved Windows Device Names (cannot be filenames)
#ifdef PLATFORM_WINDOWS
		static const std::set<std::string> reserved_names = {
			"CON",	"PRN",	"AUX",	"NUL",	"COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7",
			"COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
#endif

		const std::string illegal_chars = "<>:\"|?*";
		Filepath legal_path;

		// Iterate through each part of the path (e.g., "folder", "subfolder", "file.txt")
		for (const auto& part : path) {
			std::string name = part.string();

			// Skip root directory (e.g., "C:/") to avoid breaking drive letters
			if (part == path.root_name() || part == path.root_directory()) {
				legal_path /= part;
				continue;
			}

			// 2. Remove illegal and control characters
			for (char& c : name) {
				if (illegal_chars.find(c) != std::string::npos || (unsigned char)c < 32) {
					c = '_';
				}
			}

			// 3. Remove trailing dots and spaces
			while (!name.empty() && (name.back() == ' ' || name.back() == '.')) {
				name.pop_back();
			}

			// 4. Check for Reserved Names (e.g., "CON.txt" is illegal)
			// We check the stem (filename without extension)
#ifdef PLATFORM_WINDOWS
			std::string stem = Filepath(name).stem().string();
			std::transform(stem.begin(), stem.end(), stem.begin(), ::toupper);
			if (reserved_names.count(stem)) {
				name = "_" + name;
			}
#endif

			if (!name.empty()) {
				legal_path /= name;
			}
		}

		path = legal_path;
		return !path.empty();
	}

	void Args::parse(int argc, char* argv[]) {
		for (int i = 1; i < argc; ++i) {
			std::string arg = argv[i];
			if (arg.size() < 2 || arg.substr(0, 2) != "--") {
				CORE_WARN("format not correct: {0}, must start with --", arg);
				continue;
			}
			arg = arg.substr(2);
			size_t equal_pos = arg.find('=');
			if (equal_pos != std::string::npos) {
				std::string key = arg.substr(0, equal_pos);
				std::string value = arg.substr(equal_pos + 1);
				m_args[key] = value;
			}
			else {
				m_args[arg] = "";
			}
		}
	}

}
