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

	auto mesh = z1::g_runtime_context.m_asset_manager->get<StaticMesh>("DamagedHelmet/mesh_helmet_LP_13930damagedHelmet");

	std::cout << mesh->m_guid.value << std::endl;
	std::cout << mesh->m_primitives.size() << std::endl;

	auto image = z1::g_runtime_context.m_asset_manager->get<Image2D>("DamagedHelmet/T_0");

	std::cout << image->m_guid.value << std::endl;
	std::cout << image->get_description().m_width << "x" << image->get_description().m_height << std::endl;

	return 0;
}
