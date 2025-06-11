#pragma once

#include "core/core.h"
#include <filesystem>
#include <string>
#include <fstream>
#include <iostream>

namespace z1 {

	using Filepath = std::filesystem::path;

	enum struct API ArrayDataType {
		None = 0,
		Uint8,
		Uint16,
		Uint32,
		Uint64,
		Int8,
		Int16,
		Int32,
		Int64,
		Float16,
		Float32,
		Float64,
	};

	inline size_t data_type_to_size(ArrayDataType type);

	struct API NDArray {

		NDArray() noexcept;
		NDArray(void* data, size_t size, std::vector<uint32_t> const& shape, ArrayDataType type);

		void const* data() const noexcept;
		void* data() noexcept;
		size_t size() const noexcept;
		uint32_t ndim() const noexcept;
		std::vector<uint32_t> const& shape() const noexcept;
		ArrayDataType dtype() const noexcept;

	private:
		std::string m_data;
		std::vector<uint32_t> m_shape;
		ArrayDataType m_dtype;
	};

	struct API FileSystem {
		std::string read_file(Filepath const& path) noexcept;
		NDArray read_image(Filepath const& path) noexcept;

		bool write_file(Filepath const& path, void* data, size_t size) noexcept;
		bool write_image(Filepath const& path, void* data, size_t size, uint32_t width, uint32_t height, uint32_t channels) noexcept;

#ifdef ENGINE_DIR
		Filepath m_engine_dir = ENGINE_DIR;
#else
		Filepath m_engine_dir = "../runtime";
#endif

	};


}
