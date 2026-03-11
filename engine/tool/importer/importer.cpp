#include "z1engine.h"
#include <iostream>
#include <string>
#include <filesystem>
#include <vector>

using namespace z1;

void print_usage() {
	std::cout << "Usage: importer <input_file> <output_path_relative_to_content> [options]" << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "  --type <type>       Force import type (texture, gltf, obj)" << std::endl;
	std::cout << "  --sampler <mode>    Sampler mode (nearest, linear) [texture only]" << std::endl;
	std::cout << "  --wrap <mode>       Wrap mode (repeat, mirror, clamp, border) [texture only]" << std::endl;
}

int main(int argc, char** argv) {
	if (argc < 3) {
		print_usage();
		return 1;
	}

	g_runtime_context.m_timer = std::make_shared<Timer>();
	g_runtime_context.init_logger();
	g_runtime_context.m_file_system = std::make_shared<FileSystem>();
	g_runtime_context.m_asset_manager = std::make_shared<AssetManager>();

	std::string input_file = argv[1];
	std::string output_path = argv[2];
	std::string type = "";

	SamplerMode sampler = SamplerMode::Linear;
	WrapMode wrap = WrapMode::Repeat;

	for (int i = 3; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--type" && i + 1 < argc) {
			type = argv[++i];
		} else if (arg == "--sampler" && i + 1 < argc) {
			std::string s = argv[++i];
			if (s == "nearest") sampler = SamplerMode::Nearest;
			else if (s == "linear") sampler = SamplerMode::Linear;
		} else if (arg == "--wrap" && i + 1 < argc) {
			 std::string s = argv[++i];
			 if (s == "repeat") wrap = WrapMode::Repeat;
			 else if (s == "mirror") wrap = WrapMode::MirroredRepeat;
			 else if (s == "clamp") wrap = WrapMode::ClampToEdge;
			 else if (s == "border") wrap = WrapMode::ClampToBorder;
		}
	}

	if (type.empty()) {
		auto ext = std::filesystem::path(input_file).extension().string();
		if (ext == ".png" || ext == ".jpg" || ext == ".tga" || ext == ".bmp") type = "texture";
		else if (ext == ".glb" || ext == ".gltf") type = "gltf";
		else if (ext == ".obj") type = "obj";
	}

	ImportResult result;
	bool known_type = false;

	if (type == "texture") {
		known_type = true;
		TextureImporterSettings settings;
		settings.file = input_file;
		settings.path = output_path;
		settings.sampler_mode = sampler;
		settings.wrap_mode = wrap;
		result = TextureImporter::import(settings);
	} else if (type == "gltf") {
		known_type = true;
		GltfImporterSettings settings;
		settings.file = input_file;
		settings.path = output_path;
		result = GltfImporter::import(settings);
	} else if (type == "obj") {
		known_type = true;
		ObjImporterSettings settings;
		settings.file = input_file;
		settings.path = output_path;
		result = ObjImporter::import(settings);
	}

	if (!known_type) {
		std::cerr << "Unknown or unsupported file type: " << input_file << std::endl;
		return 1;
	}

	if (result.success) {
		std::cout << "Import successful!" << std::endl;
		for (auto& file : result.files) {
			std::cout << "Generated: " << file.string() << std::endl;
		}
		return 0;
	} else {
		std::cerr << "Import failed!" << std::endl;
		return 1;
	}
}
