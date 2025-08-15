#include "z1engine.h"

using namespace z1;

struct OurApp : Application {
	void init() override {
		for (auto const& root : g_runtime_context.m_asset_manager->get_search_roots()) {
			std::cout << root << std::endl;
		}

		terminate();
	};
};

int main() {
	std::cout << "hello world!\n";

	OurApp app;
	app.init();
	app.run();

	return 0;
}
