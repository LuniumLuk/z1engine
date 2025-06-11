#include "pch.h"
#include "core/io.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "tinyexr/tinyexr.h"

namespace z1 {

	size_t data_type_to_size(ArrayDataType type) {
		switch (type) {
		case ArrayDataType::None:
			return 0;
		case ArrayDataType::Uint8:
		case ArrayDataType::Int8:
			return 1;
		case ArrayDataType::Uint16:
		case ArrayDataType::Int16:
		case ArrayDataType::Float16:
			return 2;
		case ArrayDataType::Uint32:
		case ArrayDataType::Int32:
		case ArrayDataType::Float32:
			return 4;
		case ArrayDataType::Uint64:
		case ArrayDataType::Int64:
		case ArrayDataType::Float64:
			return 8;
		default:
			return 0;
		}
	}

	NDArray::NDArray() noexcept
		: m_data("")
		, m_shape({})
		, m_dtype(ArrayDataType::None) {}

	NDArray::NDArray(void* data, size_t size, std::vector<uint32_t> const& shape, ArrayDataType type)
		: m_shape(shape)
		, m_dtype(type) {
		size_t shapeSize = data_type_to_size(type);
		for (auto dim : shape) {
			shapeSize *= dim;
		}
		CORE_ASSERT(shapeSize == size, "data size does not match shape!");

		m_data.resize(size);
		std::copy((char*)data, (char*)data + size, m_data.begin());
	}

	void const* NDArray::data() const noexcept {
		return m_data.data();
	}

	void* NDArray::data() noexcept {
		return m_data.data();
	}

	size_t NDArray::size() const noexcept {
		return m_data.size();
	}

	uint32_t NDArray::ndim() const noexcept {
		return (uint32_t)m_shape.size();
	}

	std::vector<uint32_t> const& NDArray::shape() const noexcept {
		return m_shape;
	}

	ArrayDataType NDArray::dtype() const noexcept {
		return m_dtype;
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

	NDArray FileSystem::read_image(Filepath const& path) noexcept {
		PROFILE_FUNCTION();
		auto ext = path.extension().string();

		const std::vector<std::string> stbExtensions = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".psd", ".gif", ".pic" };
		if (std::find(stbExtensions.begin(), stbExtensions.end(), ext) != stbExtensions.end()) {
			int width, height, channels;
			stbi_set_flip_vertically_on_load(true);
			stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
			if (data) {
				NDArray ret(data, width * height * 4 * sizeof(stbi_uc), { (uint32_t)height, (uint32_t)width, 4 }, ArrayDataType::Uint8);
				stbi_image_free(data);
				return ret;
			}
			else {
				CORE_ERROR("failed to read image: {0}", path);
			}
		}

		const std::vector<std::string> exrExtensions = { ".exr" };
		if (std::find(exrExtensions.begin(), exrExtensions.end(), ext) != exrExtensions.end()) {
			float* data;
			const char* err = nullptr;
			int width, height;
			int success = LoadEXR(&data, &width, &height, path.string().c_str(), &err);
			if (success == TINYEXR_SUCCESS) {
				NDArray ret(data, width * height * 4 * sizeof(float), { (uint32_t)width, (uint32_t)height, 4 }, ArrayDataType::Float32);
				free(data);
				return ret;
			}
			else {
				CORE_ERROR("failed to read image: {0}", path);
				if (err) {
					CORE_ERROR("tinyexr error: {0}", err);
					FreeEXRErrorMessage(err);
				}
			}
		}

		CORE_ERROR("unsupported image format: {0}", path);
		return NDArray();
	}

	bool FileSystem::write_file(Filepath const& path, void* data, size_t size) noexcept {
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

	bool FileSystem::write_image(Filepath const& path, void* data, size_t size, uint32_t width, uint32_t height, uint32_t channels) noexcept {
		PROFILE_FUNCTION();
		auto ext = path.extension().string();
		std::string ret;

		const std::vector<std::string> pngExtensions = { ".png" };
		if (std::find(pngExtensions.begin(), pngExtensions.end(), ext) != pngExtensions.end()) {
			if (size != width * height * channels) {
				CORE_ERROR("invalid image data size: {0}", path);
				return false;
			}
			stbi_write_png(path.string().c_str(), width, height, channels, data, 0);
		}

		CORE_ERROR("unsupported image format: {0}", path);
		return false;
	}

}
