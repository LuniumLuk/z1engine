#include "z1engine.h"
#include "bakery.h"
#include "stb/stb_image_write.h"

using namespace z1;

struct OurApp : Application {
	void init() override {};
};

int main() {
	// override content root and shader root for test environment
	FileSystem::s_content_root = "../../engine";
	FileSystem::s_engine_root = "../../engine";

	OurApp app;
	app.init();

	Filepath cwd = std::filesystem::current_path();
	std::cout << "current working directory: " << cwd.generic_string() << std::endl;

	std::string filename = "../../temp.png";

	const unsigned char data[] = { 127, 127, 255, 255 };
	stbi_write_png(filename.c_str(), 1, 1, 4, data, 0);

	{
		TextureImporterSettings settings{};
		settings.file = "../../temp.png";
		settings.path = "texture/T_normal";
		settings.sampler_mode = SamplerMode::Nearest;
		TextureImporter().import(settings);
	}

	std::filesystem::remove(filename);

	return 0;
}
