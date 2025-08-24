#pragma once

#include "core/io.h"
#include "render/mesh.h"

namespace z1::io {

	bool file_is_obj_mesh(Filepath const& path) noexcept;

	std::shared_ptr<StaticMesh::Storage> load_obj_mesh_storage(Filepath const& path);

	std::shared_ptr<StaticMesh> load_obj_mesh(Filepath const& path);

}
