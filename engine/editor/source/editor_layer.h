#pragma once

#include <iostream>
#include <algorithm>
#include "z1engine.h"
#include "glad/glad.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui/imgui.h"
#include "imguizmo/ImGuizmo.h"
#include "gui.h"
#include "camera_ctrl.h"
#include "picking_system.h"
#include "browser.h"
#include "type_field.h"
#include "material_editor.h"
#include "stb/stb_image_write.h"
#include "scene/component/light.h"
#include "scene/prefab.h"
#include "asset/script_asset.h"
#include "python/python_script.h"
#include <yaml-cpp/yaml.h>

using namespace z1;
namespace fs = std::filesystem;

struct EditorSettings {
	std::string last_opened_scene_guid;
	bool show_light_gizmos = true;
	float light_gizmo_size = 0.1f;
	uint32_t curr_resolution = 0;
	bool show_skeleton_guizmos = true;
	float skeleton_gizmo_size = 0.1f;

	void save();
	void load();
};

struct EditorLayer : Layer {
	EditorLayer();
	~EditorLayer();

	void on_attach() override;
	void on_update(float delta_time) override;
	void on_fixed_update() override;
	void on_event(Event& event) override;
	void on_imgui_render() override;

	bool on_key_pressed(KeyPressedEvent& event);
	bool on_mouse_pressed(MouseButtonPressedEvent& event);

	void load_scene(std::shared_ptr<Scene> const& scene = nullptr);
	void save_screenshot();

private:
	EditorSettings m_settings;
	std::shared_ptr<EditorGUI> m_gui;
	std::unique_ptr<ContentBrowser> m_browser;
	MaterialEditor m_material_editor;
	bool m_picked_from_viewport = false;
	std::shared_ptr<Entity> m_selected_entity = nullptr;
	AssetMeta* m_selected_asset = nullptr;

	std::shared_ptr<PickingSystem> m_picking;

	bool m_is_using_gizmo = false;
	ImGuizmo::OPERATION m_current_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
	ImGuizmo::MODE m_current_gizmo_mode = ImGuizmo::MODE::LOCAL;
	//float m_light_gizmo_size = 0.1f;
	//bool m_show_light_gizmos = true;

	int m_fps_counter = 0;
	float m_fps_timer = 0.0;
	int m_frames_to_run = -1;
	int m_frame_count = 0;
	bool m_screenshot_on_exit = false;

	void use_editor_camera();
	void show_scene_graph();

	void show_asset_info();
	void show_settings();
	void show_stats();

	std::string get_image_info(Image* image);
	std::string get_uniform_buffer_info(UniformBuffer* buffer);

};
