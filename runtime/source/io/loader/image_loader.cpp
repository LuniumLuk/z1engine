#include "pch.h"
#include "io/loader/image_loader.h"
#include "core/asset_manager.h"
#include "util/yaml.h"

#include "bakery.h"
#include "tinyexr/tinyexr.h"

namespace z1::io {

	static bool file_is_ldr_image(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> exts = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".psd", ".gif", ".pic" };
		return std::find(exts.begin(), exts.end(), ext) != exts.end();
	}

	static bool file_is_hdr_image(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> exts = { ".exr" };
		return std::find(exts.begin(), exts.end(), ext) != exts.end();
	}

	static bool file_is_compressed_image(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> exts = { ".bin" };
		return std::find(exts.begin(), exts.end(), ext) != exts.end();
	}

	std::shared_ptr<Image2D> load_image2d_asset(Guid const& guid) {
		PROFILE_FUNCTION();
		auto meta = g_runtime_context.m_asset_manager->get_meta(guid);
		auto file = g_runtime_context.m_asset_manager->get_file_from_guid(guid);

		SamplerMode sampler_mode = SamplerMode::Linear;
		if (meta.extra["sampler_mode"]) {
			sampler_mode = static_cast<SamplerMode>(meta.extra["sampler_mode"].as<int>());
		}
		WrapMode wrap_mode = WrapMode::Repeat;
		if (meta.extra["wrap_mode"]) {
			wrap_mode = static_cast<WrapMode>(meta.extra["wrap_mode"].as<int>());
		}

		bool is_hdr = false;
		if (meta.extra["hdr"]) {
			is_hdr = meta.extra["hdr"].as<bool>();
		}

		if (is_hdr) {
			float* data;
			const char* err = nullptr;
			int width, height;
			int success = LoadEXR(&data, &width, &height, file.string().c_str(), &err);
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
				CORE_ERROR("failed to load image: {0}", file);
				if (err) {
					CORE_ERROR("tinyexr error: {0}", err);
					FreeEXRErrorMessage(err);
				}
			}
		}
		else {
			int width, height;
			auto data = bakery::load_compressed_image(file, &width, &height);
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
				CORE_ERROR("failed to load image: {0}", file);
			}
		}

		return nullptr;
	}

}
