#include "pch.h"
#include "io/image_loader.h"
#include "bakery.h"
//#include "stb/stb_image.h"
#include "tinyexr/tinyexr.h"

namespace z1::io {

	bool file_is_ldr_image(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> exts = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".psd", ".gif", ".pic" };
		return std::find(exts.begin(), exts.end(), ext) != exts.end();
	}

	bool file_is_hdr_image(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> exts = { ".exr" };
		return std::find(exts.begin(), exts.end(), ext) != exts.end();
	}

	bool file_is_compressed_image(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> exts = { ".bin" };
		return std::find(exts.begin(), exts.end(), ext) != exts.end();
	}

	std::shared_ptr<Image2D> load_image2d(
		Filepath const& path,
		SamplerMode sampler_mode,
		WrapMode wrap_mode) {
		PROFILE_FUNCTION();
		auto ext = path.extension().string();

		if (file_is_compressed_image(path)) {
			int width, height;
			auto data = bakery::load_compressed_image(path, &width, &height);
			if (data) {
				auto image = Image2D::create(
					data, width * height * 4 * sizeof(unsigned char),
					width, height,
					ImageFormat::RGBA8,
					sampler_mode, wrap_mode);
				bakery::free_loaded_data(data);
				return image;
			}
			else {
				CORE_ERROR("failed to load image: {0}", path);
			}
		}

		if (file_is_ldr_image(path)) {
			int width, height;
			auto data = bakery::load_uncompressed_image(path, &width, &height);
			if (data) {
				auto image = Image2D::create(
					data, width * height * 4 * sizeof(unsigned char),
					width, height,
					ImageFormat::RGBA8,
					sampler_mode, wrap_mode);
				bakery::free_loaded_data(data);
				return image;
			}
			else {
				CORE_ERROR("failed to load image: {0}", path);
			}
		}

		if (file_is_hdr_image(path)) {
			float* data;
			const char* err = nullptr;
			int width, height;
			int success = LoadEXR(&data, &width, &height, path.string().c_str(), &err);
			if (success == TINYEXR_SUCCESS) {
				auto image = Image2D::create(
					data, width * height * 4 * sizeof(float),
					width, height,
					ImageFormat::RGBA32F,
					sampler_mode, wrap_mode);
				free(data);
				return image;
			}
			else {
				CORE_ERROR("failed to load image: {0}", path);
				if (err) {
					CORE_ERROR("tinyexr error: {0}", err);
					FreeEXRErrorMessage(err);
				}
			}
		}

		CORE_ERROR("unsupported image format: {0}", path);
		return nullptr;
	}

}
