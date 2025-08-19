#include "z1engine.h"

using namespace z1;

struct OurApp : Application {
	void init() override {};
};

int main() {

	OurApp app;
	app.init();

	Filepath cwd = std::filesystem::current_path();
	std::cout << "current working directory: " << cwd.generic_string() << std::endl;

	auto scene = std::make_shared<Scene>();
	io::load_gltf_scene(scene, "../../editor/asset/mesh/DamagedHelmet.glb");

	return 0;
}
