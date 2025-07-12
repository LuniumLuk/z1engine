#include "pch.h"
#include "io/image_loader.h"
#include "stb/stb_image.h"
#include "tinyexr/tinyexr.h"

namespace z1::io {

	bool file_is_ldr_image(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> imageExtensions = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".psd", ".gif", ".pic" };
		return std::find(imageExtensions.begin(), imageExtensions.end(), ext) != imageExtensions.end();
	}

	bool file_is_hdr_image(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> imageExtensions = { ".exr" };
		return std::find(imageExtensions.begin(), imageExtensions.end(), ext) != imageExtensions.end();
	}

	std::shared_ptr<Image2D> load_image2d(
		Filepath const& path,
		SamplerMode sampler_mode,
		WrapMode wrap_mode) {
		PROFILE_FUNCTION();
		auto ext = path.extension().string();

		const std::vector<std::string> stbExtensions = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".psd", ".gif", ".pic" };
		if (std::find(stbExtensions.begin(), stbExtensions.end(), ext) != stbExtensions.end()) {
			int width, height, channels;
			stbi_set_flip_vertically_on_load(true);
			stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
			if (data) {
				auto image = Image2D::create(
					data, width * height * 4 * sizeof(stbi_uc),
					width, height,
					ImageFormat::RGBA8,
					sampler_mode, wrap_mode);
				stbi_image_free(data);
				return image;
			}
			else {
				CORE_ERROR("failed to load image: {0}", path);
			}
		}

		const std::vector<std::string> exrExtensions = { ".exr" };
		if (std::find(exrExtensions.begin(), exrExtensions.end(), ext) != exrExtensions.end()) {
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
