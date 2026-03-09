#include "game.h"

GameLayer::GameLayer() : Layer("Game") {}

GameLayer::~GameLayer() {}

void GameLayer::on_attach() {
	auto scene_path = Filepath(g_args.get<std::string>("scene", ""));
	if (!scene_path.empty()) {
		auto scene_guid = g_runtime_context.m_asset_manager->get_guid_from_path(scene_path);
		if (scene_guid.is_valid()) {
			auto scene = Scene::load(scene_guid);
			if (scene) {
				m_scene = scene;
				return;
			}
		}
	}

	CORE_ERROR("failed to load scene from path: {0}", scene_path.generic_string());
}

void GameLayer::on_update(float delta_time) {
	if (!m_scene)
		return;

	m_scene->on_update(delta_time);
	g_runtime_context.m_renderer_forward->draw(
		m_scene, g_runtime_context.m_graphics_context->m_swapchain_framebuffer);
}

void GameLayer::on_event(Event& event) {

}

void GameLayer::on_imgui_render() {

}
