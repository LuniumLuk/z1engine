#include <iostream>
#include <fstream>
#include "yaml-cpp/yaml.h"

int main() {
	// ===== Writing a YAML file =====
	{
		YAML::Emitter emitter;

		emitter << YAML::BeginMap;
		emitter << YAML::Key << "name" << YAML::Value << "John Doe";
		emitter << YAML::Key << "age" << YAML::Value << 30;
		emitter << YAML::Key << "skills" << YAML::Value << YAML::BeginSeq;
		emitter << "C++" << "Python" << "YAML";
		emitter << YAML::EndSeq;
		emitter << YAML::EndMap;

		std::ofstream fout("test.yaml");
		fout << emitter.c_str();
		std::cout << "YAML file written!\n";
	}

	// ===== Reading the YAML file =====
	{
		try {
			YAML::Node config = YAML::LoadFile("test.yaml");

			std::string name = config["name"].as<std::string>();
			int age = config["age"].as<int>();
			auto skills = config["skills"];

			std::cout << "\nLoaded YAML:\n";
			std::cout << "Name: " << name << "\n";
			std::cout << "Age: " << age << "\n";
			std::cout << "Skills:\n";

			for (const auto& skill : skills) {
				std::cout << "- " << skill.as<std::string>() << "\n";
			}
		} catch (const YAML::Exception& e) {
			std::cerr << "YAML Error: " << e.what() << "\n";
		}
	}

	return 0;
}
