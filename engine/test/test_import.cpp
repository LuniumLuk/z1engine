#include "z1engine.h"

using namespace z1;

struct OurApp : Application {
	void init() override {};
};

int main() {
	OurApp app;
	app.init();

	Filepath sandbox_root = FileSystem::s_content_root / "sandbox-tests" / "test_import";
	std::filesystem::remove_all(sandbox_root);

	auto import_texture = [](char const* file, char const* path, SamplerMode sampler = SamplerMode::Linear) {
		TextureImporterSettings settings{};
		settings.file = file;
		settings.path = path;
		settings.sampler_mode = sampler;
		auto result = TextureImporter::import(settings);
		return result.success;
	};

	auto import_obj = [](char const* file, char const* path) {
		ObjImporterSettings settings{};
		settings.file = file;
		settings.path = path;
		auto result = ObjImporter::import(settings);
		return result.success;
	};

	auto import_gltf = [](char const* file, char const* path) {
		GltfImporterSettings settings{};
		settings.file = file;
		settings.path = path;
		auto result = GltfImporter::import(settings);
		return result.success;
	};

	Filepath cwd = std::filesystem::current_path();
	std::cout << "current working directory: " << cwd.generic_string() << std::endl;

	{
		if (!import_texture(
			"asset/texture/sheet/roguelikeSheet_transparent.png",
			"sandbox-tests/test_import/texture/T_roguelikeSheet",
			SamplerMode::Nearest)) {
			std::filesystem::remove_all(sandbox_root);
			return 1;
		}
	}

	{
		if (!import_texture(
			"asset/texture/awesomeface.png",
			"sandbox-tests/test_import/texture/T_awesomeface")) {
			std::filesystem::remove_all(sandbox_root);
			return 1;
		}
	}

	{
		if (!import_texture(
			"asset/texture/tira-checker.jpg",
			"sandbox-tests/test_import/texture/T_tira-checker")) {
			std::filesystem::remove_all(sandbox_root);
			return 1;
		}
	}

	{
		if (!import_gltf(
			"asset/gltf/DamagedHelmet.glb",
			"sandbox-tests/test_import/DamagedHelmet")) {
			std::filesystem::remove_all(sandbox_root);
			return 1;
		}
	}

	{
		if (!import_obj(
			"asset/mesh/bunny.obj",
			"sandbox-tests/test_import/SM_bunny")) {
			std::filesystem::remove_all(sandbox_root);
			return 1;
		}
	}

	{
		if (!import_obj(
			"asset/mesh/Cube.obj",
			"sandbox-tests/test_import/SM_cube")) {
			std::filesystem::remove_all(sandbox_root);
			return 1;
		}
	}

	//std::vector<std::string> engine_meshes = {
	//	"Cone",
	//	"Cube",
	//	"Cylinder",
	//	"Sphere",
	//	"Suzanne",
	//};

	//for (auto& mesh : engine_meshes) {
	//	ObjImporterSettings settings{};
	//	settings.file = "../../asset/mesh/" + mesh + ".obj";
	//	settings.path = "mesh/SM_" + mesh;
	//	ObjImporter().import(settings);
	//}

	std::filesystem::remove_all(sandbox_root);
	return 0;
}
