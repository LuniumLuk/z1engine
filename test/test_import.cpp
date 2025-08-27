#include "z1engine.h"

using namespace z1;

struct OurApp : Application {
	void init() override {};
};

int main() {
	// override content root and shader root for test environment
	FileSystem::s_content_root = "../../content";
	FileSystem::s_engine_shader_root = "../../engine/shader";

	OurApp app;
	app.init();

	Filepath cwd = std::filesystem::current_path();
	std::cout << "current working directory: " << cwd.generic_string() << std::endl;

	{
		io::ImageImporterSettings settings{};
		settings.file = "../../asset/texture/sheet/roguelikeSheet_transparent.png";
		settings.path = "texture/T_roguelikeSheet";
		settings.sampler_mode = SamplerMode::Nearest;
		io::ImageImporter().import(settings);
	}

	{
		io::ImageImporterSettings settings{};
		settings.file = "../../asset/texture/awesomeface.png";
		settings.path = "texture/T_awesomeface";
		io::ImageImporter().import(settings);
	}

	{
		io::ImageImporterSettings settings{};
		settings.file = "../../asset/texture/tira-checker.jpg";
		settings.path = "texture/T_tira-checker";
		io::ImageImporter().import(settings);
	}

	{
		io::GltfImporterSettings settings{};
		settings.file = "../../asset/mesh/DamagedHelmet.glb";
		settings.path = "DamagedHelmet";
		io::GltfImporter().import(settings);
	}

	{
		io::ObjImporterSettings settings{};
		settings.file = "../../asset/mesh/bunny.obj";
		settings.path = "SM_bunny";
		io::ObjImporter().import(settings);
	}

	{
		io::ObjImporterSettings settings{};
		settings.file = "../../asset/fireplace_room/fireplace_room.obj";
		settings.path = "SM_fireplace_room";
		io::ObjImporter().import(settings);
	}

	return 0;
}
