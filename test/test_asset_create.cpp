#include "z1engine.h"

using namespace z1;

struct OurApp : Application {
	void init() override {};
};

int main() {
	// override content root and shader root for test environment
	FileSystem::s_content_root = "../../content";
	FileSystem::s_engine_root = "../../engine";

	OurApp app;
	app.init();

	Filepath cwd = std::filesystem::current_path();
	std::cout << "current working directory: " << cwd.generic_string() << std::endl;

	auto mi = MaterialInstance::create("material/MI_unlit_sample", g_runtime_context.m_asset_manager->get<Material>("material/M_unlit"));
	mi->m_override_variables["s_base_color"].default_value.valid = true;
	auto guid = g_runtime_context.m_asset_manager->get_guid_from_path("texture/T_awesomeface");
	mi->m_override_variables["s_base_color"].default_value.tex2D = g_runtime_context.m_asset_manager->get<Texture2D>(guid);
	mi->save();

	return 0;
}
