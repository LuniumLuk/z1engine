#pragma once

#include "core/io.h"
#include "asset/serializer/serializer.h"
#include "asset/serializer/binary_file.h"
#include "asset/mesh.h"

namespace z1 {

	struct StaticMeshSerializer : Serializer<StaticMeshSerializer, StaticMesh::Storage> {

		static bool serialize(Filepath const& path, std::shared_ptr<StaticMesh::Storage> const& asset);

		static std::shared_ptr<StaticMesh::Storage> deserialize(Filepath const& path);

	};

	// helper function to load static mesh asset from guid
	// this function will call asset manager to get the file path from guid
	// then call StaticMeshSerializer::deserialize to load the storage
	// finally create a StaticMesh from the storage and return it
	std::shared_ptr<StaticMesh> load_static_mesh_asset(Guid const& guid);

}
