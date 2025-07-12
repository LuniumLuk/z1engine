#include "core/io.h"
#include "render/mesh.h"

namespace z1::io {

	bool file_is_static_mesh(Filepath const& path) noexcept;

	std::shared_ptr<StaticMesh> load_static_mesh(Filepath const& path);

}
