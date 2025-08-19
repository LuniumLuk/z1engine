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

	{
		auto storage = io::load_obj_mesh_storage("../../editor/asset/fireplace_room/fireplace_room.obj");
		io::save_static_mesh_storage("fireplace_room.bin", storage);
	}

	{
		auto storage = io::load_obj_mesh_storage("../../editor/asset/mesh/bunny.obj");
		io::save_static_mesh_storage("bunny.bin", storage);
	}

	return 0;
}
