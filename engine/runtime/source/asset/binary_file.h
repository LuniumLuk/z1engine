#pragma once

#include "core/core.h"
#include "core/io.h"
#include <string>

#define BINARY_FILE_MAGIC 0x5A314246 // 'Z1BF'

namespace z1 {

#pragma pack(push, 1)
	struct BinaryFileHeader {
		uint32_t magic = BINARY_FILE_MAGIC;
		uint32_t version = 1;
		uint64_t yaml_size = 0;
		uint64_t data_size = 0;
	};
#pragma pack(pop)

	struct BinaryFile {

		struct DataSlice {
			const void* ptr = nullptr;
			size_t size = 0;

			bool valid() const noexcept { return ptr != nullptr && size > 0; }
		};

		bool load(Filepath const& file);
		bool save(Filepath const& file) const;

		void set_yaml(std::string const& yaml);
		std::string const& get_yaml() const;

		void reserve(size_t size);
		void set_data(const void* data, size_t size, size_t offset = 0);
		const size_t get_data_size() const;
		std::vector<uint8_t> const& get_data() const;
		DataSlice get_data_slice(size_t offset = 0, size_t size = WHOLE_SIZE) const;

	private:
		std::string m_yaml;
		std::vector<uint8_t> m_data;
	};

}
