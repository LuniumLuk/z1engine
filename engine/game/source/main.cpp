#include <iostream>
#include "z1engine.h"
#include "editor_layer.h"
#include "game.h"

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

LogLevel get_log_level(std::string const& level) {
	if (level == "trace") return LogLevel::Trace;
	if (level == "debug") return LogLevel::Debug;
	if (level == "info") return LogLevel::Info;
	if (level == "warn") return LogLevel::Warn;
	if (level == "error") return LogLevel::Error;
	if (level == "critical") return LogLevel::Critical;
	return LogLevel::Info;
}

int main(int argc, char* argv[]) {
	auto start = std::chrono::high_resolution_clock::now();
	CLIENT_INFO("welcome to my sekai ... ^_^");

	g_args.parse(argc, argv);

	auto log_level = get_log_level(g_args.get<std::string>("log-level", "info"));
	g_runtime_context.m_logger->set_core_log_level(log_level);
	g_runtime_context.m_logger->set_client_log_level(log_level);

	Application* app = nullptr;
	if (g_args.get<bool>("game", false)) {
		app = new GameApp();
	}
	else {
		app = new EditorApp();
	}

	app->init();

	auto end = std::chrono::high_resolution_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		end - start
	).count();

	CLIENT_INFO("app launch (ms): {0}", ms);

	app->run();
	delete app;

	return 0;
}
