#pragma once

#include "z1engine.h"
#include "imgui.h"

using namespace z1;

constexpr uint32_t g_viewport_resolutions[][2] = {
	{ 640, 480 },
	{ 800, 600 },
	{ 1024, 768 },
	{ 1280, 720 },
	{ 1280, 800 },
	{ 1280, 960 },
	{ 1280, 1024 },
	{ 1360, 768 },
	{ 1366, 768 },
	{ 1400, 1050 },
	{ 1440, 900 },
	{ 1600, 900 },
	{ 1600, 1200 },
	{ 1680, 1050 },
	{ 1920, 1080 },
	{ 1920, 1200 },
	{ 2560, 1440 },
	{ 2560, 1600 },
	{ 3840, 2160 },
};

struct EditorGUI {
	EditorGUI();
	~EditorGUI();

	void draw();

	std::shared_ptr<Framebuffer> const& get_viewport_framebuffer() const { return m_viewport_framebuffer; }
	bool is_viewport_focused() const { return m_is_viewport_focused; }
	bool is_viewport_hovered() const { return m_is_viewport_hovered; }
	void get_mouse_cursor_on_viewport(float* x, float* y) const {
		*x = m_viewport_mouse_x;
		*y = m_viewport_mouse_y;
	}

	std::function<void()> m_draw_viewport_overlay_func = nullptr;
	std::function<void()> m_draw_menu_bar_items_func = nullptr;

	bool m_dockspace = true;
	bool m_window_viewport = true;
	bool m_is_viewport_focused = false;
	bool m_is_viewport_hovered = false;
	uint32_t m_current_resolution = 0;
	std::shared_ptr<Framebuffer> m_viewport_framebuffer;
	float m_viewport_pixel_scale_x = 0.0f;
	float m_viewport_mouse_x = 0.0f;
	float m_viewport_mouse_y = 0.0f;
};

std::string open_file_dialog(char const* filter = "All Files\0*.*\0");
std::string save_file_dialog(char const* filter = "All Files\0*.*\0");
