#pragma once

#include "z1engine.h"

using namespace z1;

struct GameLayer : Layer {
	GameLayer();
	~GameLayer();

	void on_attach() override;
	void on_update(float delta_time) override;
	void on_event(Event& event) override;
	void on_imgui_render() override;

	std::shared_ptr<Scene> m_scene;
};

struct GameApp : Application {
	void init() override { push_layer(std::make_shared<GameLayer>()); }
};
