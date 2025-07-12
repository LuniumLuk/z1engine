#include "pch.h"
#include "core/io.h"

namespace z1 {

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

}
