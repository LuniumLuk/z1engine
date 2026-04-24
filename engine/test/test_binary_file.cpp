#include "z1engine.h"
#include "yaml-cpp/yaml.h"

using namespace z1;

int main() {
	auto path = std::filesystem::temp_directory_path() / "z1engine_test_binary_file.bin";
	auto cleanup = [&]() {
		std::error_code ec;
		std::filesystem::remove(path, ec);
	};

	// --- test save ---
	{
		BinaryFile file{};

		std::vector<int> data = {
			1, 1, 4, 5, 1, 4
		};

		file.set_data(data.data(), data.size() * sizeof(int));

		YAML::Emitter yaml;
		yaml << YAML::BeginMap;
		yaml << YAML::Key << "hello" << YAML::Value << "world";
		yaml << YAML::Key << "good" << YAML::Value << "bye";
		yaml << YAML::EndMap;

		file.set_yaml(yaml.c_str());
		if (!file.save(path.string())) return 1;
	}

	// --- test load ---
	{
		BinaryFile file{};
		if (!file.load(path.string())) {
			cleanup();
			return 1;
		}

		YAML::Node yaml = YAML::Load(file.get_yaml());
		if (!yaml["hello"] || !yaml["good"]) {
			cleanup();
			return 1;
		}
		if (yaml["hello"].as<std::string>() != "world") {
			cleanup();
			return 1;
		}
		if (yaml["good"].as<std::string>() != "bye") {
			cleanup();
			return 1;
		}

		std::cout << "loaded yaml:\n";
		std::cout << "hello: " << yaml["hello"].as<std::string>() << std::endl;
		std::cout << "good: " << yaml["good"].as<std::string>() << std::endl;

		std::vector<int> data(6);
		std::memcpy(data.data(), file.get_data().data(), file.get_data_size());
		std::vector<int> expected = { 1, 1, 4, 5, 1, 4 };
		if (data != expected) {
			cleanup();
			return 1;
		}

		std::cout << "loaded data:\n";
		for (auto v : data) {
			std::cout << v << ", ";
		}

	}

	cleanup();

	return 0;
}
