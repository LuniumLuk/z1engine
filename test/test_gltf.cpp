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
	settings.source_path = "../../editor/asset/mesh/DamagedHelmet.glb";
	settings.target_path = "DamagedHelmet";
	settings.cache_dir = "cache";

	io::GltfImporter::import(settings);

	return 0;
}
