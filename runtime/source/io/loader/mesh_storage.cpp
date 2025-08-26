#include "pch.h"
#include "io/loader/mesh_storage.h"
#include "core/core.h"
#include "core/asset_manager.h"
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
		BinaryFile file{};

		if (!file.load(path)) {
			CORE_ERROR("failed to load static mesh storage: {0}", path.generic_string());
			return nullptr;
		}

		YAML::Node node = YAML::Load(file.get_yaml());
		auto storage = std::make_shared<StaticMesh::Storage>();

		storage->guid.value = node["guid"].as<std::string>();
		YAML::convert<glm::vec3>::decode(node["bound_min"], storage->bound_min);
		YAML::convert<glm::vec3>::decode(node["bound_max"], storage->bound_max);
		auto vertex_count = node["vertex_count"].as<size_t>();
		auto index_count = node["index_count"].as<size_t>();

		for (auto const& prim_node : node["primitives"]) {
			StaticMesh::Primitive::Storage prim_storage{};
			prim_storage.index_start = prim_node["index_start"].as<uint32_t>();
			prim_storage.index_count = prim_node["index_count"].as<uint32_t>();
			prim_storage.vertex_count = prim_node["vertex_count"].as<uint32_t>();
			YAML::convert<glm::vec3>::decode(prim_node["bound_min"], prim_storage.bound_min);
			YAML::convert<glm::vec3>::decode(prim_node["bound_max"], prim_storage.bound_max);
			prim_storage.material.value = prim_node["guid"].as<std::string>();
			prim_storage.has_indices = prim_node["has_indices"].as<bool>();
			prim_storage.has_normal = prim_node["has_normal"].as<bool>();
			prim_storage.has_tangent = prim_node["has_tangent"].as<bool>();
			storage->primitives.push_back(prim_storage);
		}

		storage->vertices.resize(vertex_count);
		storage->indices.resize(index_count);

		auto vdata_size = vertex_count * sizeof(StaticMesh::VertexData);
		auto idata_size = index_count * sizeof(uint32_t);

		auto vdata_slice = file.get_data_slice(0, vdata_size);
		if (vdata_slice.size != vdata_size) {
			CORE_ERROR("corrupted static mesh storage: {0}", path.generic_string());
			return nullptr;
		}
		std::memcpy(storage->vertices.data(), vdata_slice.ptr, vdata_slice.size);

		auto idata_slice = file.get_data_slice(vdata_size, idata_size);
		if (idata_slice.size != idata_size) {
			CORE_ERROR("corrupted static mesh storage: {0}", path.generic_string());
			return nullptr;
		}
		std::memcpy(storage->indices.data(), idata_slice.ptr, idata_slice.size);

		return storage;
	}

	std::shared_ptr<StaticMesh> load_static_mesh_asset(Guid const& guid) {
		PROFILE_FUNCTION();
		auto meta = g_runtime_context.m_asset_manager->get_meta(guid);
		auto file = g_runtime_context.m_asset_manager->get_file_from_guid(guid);

		auto storage = load_static_mesh_storage(file);
		if (!storage) {
			CORE_ERROR("failed to load static mesh storage: {0}", file.generic_string());
			return nullptr;
		}

		return std::make_shared<StaticMesh>(storage);
	}

}
