#include "pch.h"
#include "io/binary_file.h"

namespace z1 {

	bool BinaryFile::load(Filepath const& path) {
		std::ifstream ifs(path, std::ios::binary);
		if (!ifs) {
			CORE_ERROR("failed to read from file {0}", path);
			return false;
		}

		BinaryFileHeader header;
		ifs.read(reinterpret_cast<char*>(&header), sizeof(header));

		if (header.magic != BINARY_FILE_MAGIC) {
			CORE_ERROR("invalid magic number in {0}", path);
			return false;
		}

		m_yaml.resize(header.yaml_size);
		ifs.read(m_yaml.data(), header.yaml_size);

		m_data.resize(header.data_size);
		ifs.read(reinterpret_cast<char*>(m_data.data()), header.data_size);

		return true;
	}

	bool BinaryFile::dump(Filepath const& path) const {
		std::ofstream ofs(path, std::ios::binary);
		if (!ofs) {
			CORE_ERROR("failed to write to file {0}", path);
			return false;
		}

		BinaryFileHeader header;
		header.yaml_size = m_yaml.size();
		header.data_size = m_data.size();

		ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));
		ofs.write(m_yaml.data(), m_yaml.size());
		ofs.write(reinterpret_cast<const char*>(m_data.data()), m_data.size());

		return true;
	}

	void BinaryFile::set_yaml(std::string const& yaml) {
		m_yaml = yaml;
	}

	std::string const& BinaryFile::get_yaml() const {
		return m_yaml;
	}

	void BinaryFile::set_data(const void* data, size_t size) {
		m_data.resize(size);
		std::memcpy(m_data.data(), data, size);
			}

	const size_t BinaryFile::get_data_size() const {
		return m_data.size();
	}

	const std::vector<uint8_t>& BinaryFile::get_data() const {
		return m_data;
	}

	void const* BinaryFile::get_data_ptr(size_t offset, size_t size) const {
		if (size == WHOLE_SIZE) {
			size = get_data_size();
		}

		if (offset + size > m_data.size()) {
			CORE_ERROR("out of range");
			return nullptr;
		}

		return &m_data[offset];
	}

}
