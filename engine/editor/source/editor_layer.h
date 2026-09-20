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
#include "quality_preset.h"
#include "stb/stb_image_write.h"
#include "scene/component/light.h"
#include "scene/prefab.h"
#include "asset/script_asset.h"
#include "python/python_script.h"
#include <yaml-cpp/yaml.h>

using namespace z1;
namespace fs = std::filesystem;

// Global settings storage the editor works against, toggled from the global settings panel:
// `editor` uses editor_settings.yaml, `scene` uses the scene file.
enum class GlobalsSource : int {
	Editor = 0,
	Scene = 1,
};

struct EditorSettings {
	std::string last_opened_scene_guid;
	bool show_light_gizmos = true;
	float light_gizmo_size = 0.1f;
	uint32_t curr_resolution = 0;
	bool show_skeleton_guizmos = true;
	float skeleton_gizmo_size = 0.1f;
	QualityPreset quality_preset = QualityPreset::High;
	// where the global settings live: `editor` keeps them in this file only, `scene` loads
	// them from the scene file and flushes them back there on save scene
	GlobalsSource globals_source = GlobalsSource::Editor;
	// editor globals snapshot (same reflected key set as a scene's global_settings block)
	YAML::Node globals;

	void save();
	void load();
	void apply_globals(GlobalSettings& target) const;
	void capture_globals(GlobalSettings& source);
};

struct EditorLayer : Layer {
	EditorLayer();
	~EditorLayer();

	void on_attach() override;
	void on_detach() override;
	void on_update(float delta_time) override;
	void on_fixed_update() override;
	void on_event(Event& event) override;
	void on_imgui_render() override;

	bool on_key_pressed(KeyPressedEvent& event);
	bool on_mouse_pressed(MouseButtonPressedEvent& event);

	void load_scene(std::shared_ptr<Scene> const& scene = nullptr);
	void save_scene();
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
	bool m_settings_saved = false;

	void use_editor_camera();
	void show_scene_graph();

	void show_asset_info();
	void show_settings();
	void show_globals_source_toggle();
	void show_quality_preset_selector();
	void show_stats();
	void persist_settings();

	std::string get_image_info(Image* image);
	std::string get_uniform_buffer_info(UniformBuffer* buffer);

};
