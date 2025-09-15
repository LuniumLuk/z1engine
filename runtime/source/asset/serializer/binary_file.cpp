#include "pch.h"
#include "asset/serializer/binary_file.h"

namespace z1 {

	bool BinaryFile::load(Filepath const& path) {
		std::ifstream ifs(path, std::ios::binary);
		if (!ifs) {
			CORE_ERROR("failed to read from file {0}", path);
			return false;
		}

		BinaryFileHeader header;
		if (!ifs.read(reinterpret_cast<char*>(&header), sizeof(header))) {
			CORE_ERROR("failed to read header from {0}", path);
			return false;
		}

		if (header.magic != BINARY_FILE_MAGIC) {
			CORE_ERROR("invalid magic number in {0}", path);
			return false;
		}

		if (header.version != 1) {
			CORE_ERROR("unsupported version {} in {}", header.version, path);
			return false;
		}

		m_yaml.resize(header.yaml_size);
		if (!ifs.read(m_yaml.data(), header.yaml_size)) {
			CORE_ERROR("failed to read yaml section from {}", path);
			return false;
		}

		m_data.resize(header.data_size);
		if (!ifs.read(reinterpret_cast<char*>(m_data.data()), header.data_size)) {
			CORE_ERROR("failed to read data section from {}", path);
			return false;
		}

		return true;
	}

	bool BinaryFile::save(Filepath const& path) const {
		std::ofstream ofs(path, std::ios::binary);
		if (!ofs) {
			CORE_ERROR("failed to write to file {0}", path);
			return false;
		}

		BinaryFileHeader header;
		header.yaml_size = static_cast<uint64_t>(m_yaml.size());
		header.data_size = static_cast<uint64_t>(m_data.size());

		ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));
		ofs.write(m_yaml.data(), m_yaml.size());
		ofs.write(reinterpret_cast<const char*>(m_data.data()), m_data.size());

		if (!ofs.good()) {
			CORE_ERROR("failed to write fully to {}", path);
			return false;
		}

		return true;
	}

	void BinaryFile::set_yaml(std::string const& yaml) {
		m_yaml = yaml;
	}

	std::string const& BinaryFile::get_yaml() const {
		return m_yaml;
	}

	void BinaryFile::reserve(size_t size) {
		m_data.reserve(size);
	}

	void BinaryFile::set_data(const void* data, size_t size, size_t offset) {
		if (size == 0) return;
		if (data == nullptr) {
			CORE_ERROR("null data pointer with nonzero size");
			return;
		}
		m_data.resize(offset + size);
		std::memcpy(m_data.data() + offset, data, size);
	}

	const size_t BinaryFile::get_data_size() const {
		return m_data.size();
	}

	std::vector<uint8_t> const& BinaryFile::get_data() const {
		return m_data;
	}

	BinaryFile::DataSlice BinaryFile::get_data_slice(size_t offset, size_t size) const {
		if (size == WHOLE_SIZE) {
			size = get_data_size();
		}

		if (offset > m_data.size() || offset + size > m_data.size()) {
			CORE_ERROR("out of range: offset {} size {} (data size {})", offset, size, m_data.size());
			return {};
		}

		return { m_data.data() + offset, size };
	}

}
