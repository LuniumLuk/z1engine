#include "pch.h"
#include "core/io.h"

namespace z1 {

	Filepath FileSystem::s_content_root = "content";
	Filepath FileSystem::s_cache_root = "cache";
	Filepath FileSystem::s_engine_root = "engine";

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

}
