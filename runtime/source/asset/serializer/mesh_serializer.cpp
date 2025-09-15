#include "pch.h"
#include "asset/serializer/mesh_serializer.h"
#include "core/core.h"
#include "asset/asset_manager.h"
#include "util/yaml.h"

namespace z1 {

	bool StaticMeshSerializer::serialize(Filepath const& path, std::shared_ptr<StaticMesh::Storage> const& storage) {
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
		yaml << YAML::Key << "vertex_count" << YAML::Value << storage->vertices.size();
		yaml << YAML::Key << "index_count" << YAML::Value << storage->indices.size();
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

	std::shared_ptr<StaticMesh::Storage> StaticMeshSerializer::deserialize(Filepath const& path) {
		BinaryFile file{};

		if (!file.load(path)) {
			CORE_ERROR("failed to load static mesh storage: {0}", path.generic_string());
			return nullptr;
		}

		YAML::Node node = YAML::Load(file.get_yaml());
		auto storage = std::make_shared<StaticMesh::Storage>();

		storage->guid.value = node["guid"].as<std::string>();
		storage->bound_min = node["bound_min"].as<glm::vec3>();
		storage->bound_max = node["bound_max"].as<glm::vec3>();
		auto vertex_count = node["vertex_count"].as<size_t>();
		auto index_count = node["index_count"].as<size_t>();

		for (auto const& prim_node : node["primitives"]) {
			StaticMesh::Primitive::Storage prim_storage{};
			prim_storage.index_start = prim_node["index_start"].as<uint32_t>();
			prim_storage.index_count = prim_node["index_count"].as<uint32_t>();
			prim_storage.vertex_count = prim_node["vertex_count"].as<uint32_t>();
			prim_storage.bound_min = prim_node["bound_min"].as<glm::vec3>();
			prim_storage.bound_max = prim_node["bound_max"].as<glm::vec3>();
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

		auto storage = StaticMeshSerializer::deserialize(file);
		if (!storage) {
			CORE_ERROR("failed to load static mesh storage: {0}", file.generic_string());
			return nullptr;
		}

		return std::make_shared<StaticMesh>(storage);
	}

}
