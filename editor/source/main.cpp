#include <iostream>
#include <z1engine.h>
#include <glad/glad.h>

using namespace z1;

struct MyLayer : Layer {
    void on_update(double deltaTime) override {
        glClearColor(1.0f, 0.8f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
};

struct MyApp : Application {
    void init() override {
        push_layer(std::make_shared<MyLayer>());
    };
};

int main() {
    std::cout << "hello world!\n";

    MyApp app;

    app.init();

    app.run();

    return 0;
}
