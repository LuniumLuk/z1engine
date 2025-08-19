#include "core/io.h"
#include "scene/scene.h"

namespace z1::io {

	bool file_is_gltf(Filepath const& path) noexcept;

	void load_gltf_scene(std::shared_ptr<Scene> const& scene, Filepath const& path);

}
