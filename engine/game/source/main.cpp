#include <iostream>
#include "z1engine.h"
#include "editor_layer.h"

using namespace z1;

struct EditorApp : Application {
	void init() override {
		fs::path ini_path = "imgui.ini";
		if (!fs::exists(ini_path)) {
			fs::path default_path = "engine/config/default.ini";
			if (fs::exists(default_path)) {
				fs::copy_file(default_path, ini_path);
			}
		}

		push_layer(std::make_shared<EditorLayer>());
	};
};

int main(int argc, char* argv[]) {
	auto start = std::chrono::high_resolution_clock::now();
	std::cout << "hello world!\n";

	g_args.parse(argc, argv);

	EditorApp app;
	app.init();

	auto end = std::chrono::high_resolution_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		end - start
	).count();

	std::cout << "app launch (ms): " << ms << "\n";

	app.run();

	return 0;
}
