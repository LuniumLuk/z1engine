#include "pch.h"
#include "io/loader/mesh_storage.h"
#include "util/yaml.h"

namespace z1::io {

	bool save_static_mesh_storage(Filepath const& path, std::shared_ptr<StaticMesh::Storage> const& storage) {
		BinaryFile file{};

		auto vdata_size = storage->vertices.size() * sizeof(StaticMesh::VertexData);
		auto idata_size = storage->indices.size() * sizeof(uint32_t);

		file.reserve(vdata_size + idata_size);
		file.set_data(storage->vertices.data(), vdata_size, 0);
		file.set_data(storage->indices.data(), idata_size, vdata_size);

		YAML::Emitter yaml;
		yaml << YAML::BeginMap;
		yaml << YAML::Key << "guid" << YAML::Value << storage->guid.value;
		yaml << YAML::Key << "bound_min" << YAML::Value << storage->bound_min;
		yaml << YAML::Key << "bound_max" << YAML::Value << storage->bound_max;
		yaml << YAML::Key << "primitives" << YAML::Value << YAML::BeginSeq;

		for (auto const& prim : storage->primitives) {
			yaml << YAML::BeginMap;
			yaml << YAML::Key << "index_start" << YAML::Value << prim.index_start;
			yaml << YAML::Key << "index_count" << YAML::Value << prim.index_count;
			yaml << YAML::Key << "vertex_count" << YAML::Value << prim.vertex_count;
			yaml << YAML::Key << "bound_min" << YAML::Value << prim.bound_min;
			yaml << YAML::Key << "bound_max" << YAML::Value << prim.bound_max;
			yaml << YAML::Key << "guid" << YAML::Value << prim.material.value;
			yaml << YAML::Key << "has_indices" << YAML::Value << prim.has_indices;
			yaml << YAML::Key << "has_normal" << YAML::Value << prim.has_normal;
			yaml << YAML::Key << "has_tangent" << YAML::Value << prim.has_tangent;
			yaml << YAML::EndMap;
		}

		yaml << YAML::EndMap;

		std::cout << "----yaml----\n";
		std::cout << yaml.c_str() << std::endl;
		std::cout << "----yaml----\n";

		file.set_yaml(yaml.c_str());
		return file.save(path);
	}

	std::shared_ptr<StaticMesh::Storage> load_static_mesh_storage(Filepath const& path) {
		return nullptr;
	}

}
