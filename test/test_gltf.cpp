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

	io::GltfImporterSettings settings{};
	settings.name = "DamagedHelmet";
	settings.file = "../../editor/asset/mesh/DamagedHelmet.glb";
	settings.path = "DamagedHelmet";
	settings.root = "content";

	io::GltfImporter::import(settings);

	return 0;
}
