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
				auto const& cam = scene->get_main_camera();
				if (!cam) {
					CORE_ERROR("No main camera found in the scene!");
					terminate();
					return;
				}

				g_runtime_context.m_scene = scene;
				g_runtime_context.m_global->anim_enabled = true;
				g_runtime_context.m_global->script_enabled = true;
				return;
			}
		}
	}

	CORE_ERROR("failed to load scene from path: {0}", scene_path.generic_string());
	terminate();
}

void GameLayer::on_update(float delta_time) {
	if (!g_runtime_context.m_scene)
		return;

	g_runtime_context.m_scene->on_update(delta_time);
	if (g_runtime_context.m_global->render_mode == RenderMode::Deferred) {
		g_runtime_context.m_renderer_deferred->draw(
			g_runtime_context.m_scene, g_runtime_context.m_graphics_context->m_swapchain_framebuffer);
	}
	else {
		g_runtime_context.m_renderer_forward->draw(
			g_runtime_context.m_scene, g_runtime_context.m_graphics_context->m_swapchain_framebuffer);
	}
}

void GameLayer::on_event(Event& event) {

}

void GameLayer::on_imgui_render() {

}
