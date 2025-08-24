#pragma once

#include "core/io.h"
#include "io/binary_file.h"
#include "render/mesh.h"

namespace z1::io {

	bool save_static_mesh_storage(Filepath const& path, std::shared_ptr<StaticMesh::Storage> const& storage);

	std::shared_ptr<StaticMesh::Storage> load_static_mesh_storage(Filepath const& path);

}
