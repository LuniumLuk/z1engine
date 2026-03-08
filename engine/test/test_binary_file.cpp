#include "z1engine.h"
#include "yaml-cpp/yaml.h"

using namespace z1;

int main() {

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
		file.save("test.bin");
	}

	// --- test load ---
	{
		BinaryFile file{};
		if (!file.load("test.bin")) return 1;

		YAML::Node yaml = YAML::Load(file.get_yaml());

		std::cout << "loaded yaml:\n";
		std::cout << "hello: " << yaml["hello"].as<std::string>() << std::endl;
		std::cout << "good: " << yaml["good"].as<std::string>() << std::endl;

		std::vector<int> data(6);
		std::memcpy(data.data(), file.get_data().data(), file.get_data_size());

		std::cout << "loaded data:\n";
		for (auto v : data) {
			std::cout << v << ", ";
		}

	}

	return 0;
}
