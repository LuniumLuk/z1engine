#include "baker/image.h"
#include "logger.h"
#include "stb/stb_image.h"
#include "stb/stb_image_resize.h"
#include "lz4/lz4.h"

namespace z1::bakery {

	static bool is_supported_format(std::filesystem::path const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> exts = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".psd", ".gif", ".pic" };
		return std::find(exts.begin(), exts.end(), ext) != exts.end();
	}

	bool compress_image(std::filesystem::path const& input, std::filesystem::path const& output) {
		int width, height;
		auto data = load_uncompressed_image(input, &width, &height);

		auto res = compress_image_data(output, data, width, height);

		free_loaded_data(data);
		return res;
	}

	bool compress_image_data(std::filesystem::path const& output, unsigned char const* data, int width, int height) {
		// compress with LZ4
		int uncompressed_size = width * height * 4;
		int max_compressed_size = LZ4_compressBound(uncompressed_size);
		char* compressed_data = (char*)malloc(max_compressed_size);
		if (!compressed_data) {
			log_error("failed to allocate memory for compressed data");
			return false;
		}

		int compressed_size = LZ4_compress_default(
			(const char*)data, compressed_data, uncompressed_size, max_compressed_size
		);
		if (!compressed_size) {
			log_error("failed to compress image data");
			return false;
		}

		// save to custom binary file (width, height, compressed data)
		std::filesystem::create_directories(output.parent_path());
		FILE* f = fopen(output.generic_string().c_str(), "wb");
		if (!f) {
			log_error("failed to open output file");
			log_error(output.generic_string());
			return false;
		}
		fwrite(&width, sizeof(int), 1, f);
		fwrite(&height, sizeof(int), 1, f);
		fwrite(&compressed_size, sizeof(int), 1, f);
		fwrite(compressed_data, 1, compressed_size, f);
		fclose(f);

		// cleanup
		free(compressed_data);
		return true;
	}

	const unsigned char* load_uncompressed_image(std::filesystem::path const& path, int* width, int* height) {
		if (!is_supported_format(path)) {
			log_error("unsupported image format: " + path.string());
			return false;
		}

		// load image data
		int channels;
		stbi_set_flip_vertically_on_load(true);
		stbi_uc* data = stbi_load(path.string().c_str(), width, height, &channels, STBI_rgb_alpha);
		if (!data) {
			log_error("failed to load image:" + path.string());
			return false;
		}

		return data;
	}

	const unsigned char* load_compressed_image(std::filesystem::path const& path, int* width, int* height) {
		// read binary file
		FILE* f = fopen(path.string().c_str(), "rb");
		if (!f) {
			log_error("failed to open file: " + path.string());
			return nullptr;
		}

		int compressed_size;
		fread(width, sizeof(int), 1, f);
		fread(height, sizeof(int), 1, f);
		fread(&compressed_size, sizeof(int), 1, f);

		char* compressed_data = (char*)malloc(compressed_size);
		fread(compressed_data, 1, compressed_size, f);
		fclose(f);

		// decompress with LZ4
		int uncompressed_size = (*width) * (*height) * 4;
		unsigned char* pixels = (unsigned char*)malloc(uncompressed_size);
		LZ4_decompress_safe(compressed_data, (char*)pixels, compressed_size, uncompressed_size);

		// Cleanup
		free(compressed_data);

		return pixels;
	}

	void free_loaded_data(const unsigned char* data) noexcept {
		if (data) {
			free((void*)data);
		}
		else {
			log_warning("attempted to free null pointer in free_compressed_image");
		}
	}

}
