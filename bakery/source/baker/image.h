#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace z1::bakery {

	bool compress_image(std::filesystem::path const& input, std::filesystem::path const& output);

	const unsigned char* load_uncompressed_image(std::filesystem::path const& path, int* width, int* height);
	const unsigned char* load_compressed_image(std::filesystem::path const& path, int* width, int* height);

	void free_loaded_data(const unsigned char* data) noexcept;

}
