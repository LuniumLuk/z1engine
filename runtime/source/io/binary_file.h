//#include <cstdint>
//#include <string>
//#include <vector>
#pragma once

#include "core/core.h"
#include "core/io.h"
#include <string>

namespace z1 {

	struct BinaryFileHeader {
		uint32_t magic = 0x42494E46; // 'BINF'
		uint32_t version = 1;
		uint64_t yaml_size = 0;
		uint64_t data_size = 0;
	};

	struct BinaryFile {

		bool load(Filepath const& path);
		bool dump(Filepath const& path) const;

		void set_yaml(std::string const& yaml);
		std::string const& get_yaml() const;

		void set_data(const void* data, size_t size);
		const size_t get_data_size() const;
		const std::vector<uint8_t>& get_data() const;
		void const* get_data_ptr(size_t offset = 0, size_t size = WHOLE_SIZE) const;

	private:
		std::string m_yaml;
		std::vector<uint8_t> m_data;
	};

}
