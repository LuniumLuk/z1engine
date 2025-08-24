#include "z1engine.h"

using namespace z1;

struct OurApp : Application {
	void init() override {
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
