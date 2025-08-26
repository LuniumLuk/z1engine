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

	z1::g_runtime_context.m_asset_manager->get_content_roots();

	{
		io::GltfImporterSettings settings{};
		settings.file = "../../editor/asset/mesh/DamagedHelmet.glb";
		settings.path = "DamagedHelmet";
		settings.root = "content";
		io::GltfImporter().import(settings);
	}

	{
		io::ImageImporterSettings settings{};
		settings.file = "../../editor/asset/texture/roguelikeSheet_transparent.png";
		settings.path = "roguelikeSheet_transparent";
		settings.root = "content";
		io::ImageImporter().import(settings);
	}

	return 0;
}
