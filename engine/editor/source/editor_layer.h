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
};

inline void save_editor_settings(EditorSettings const& settings) {
	YAML::Emitter yaml;
	yaml << YAML::BeginMap;
	yaml << YAML::Key << "last_opened_scene_guid" << YAML::Value << settings.last_opened_scene_guid;
	yaml << YAML::Key << "show_light_gizmos" << YAML::Value << settings.show_light_gizmos;
	yaml << YAML::Key << "light_gizmo_size" << YAML::Value << settings.light_gizmo_size;
	yaml << YAML::Key << "curr_resolution" << YAML::Value << settings.curr_resolution;
	yaml << YAML::Key << "show_skeleton_guizmos" << YAML::Value << settings.show_skeleton_guizmos;
	yaml << YAML::Key << "skeleton_gizmo_size" << YAML::Value << settings.skeleton_gizmo_size;
	yaml << YAML::EndMap;

	std::ofstream fout("editor_settings.yaml");
	fout << yaml.c_str();
}

inline EditorSettings load_editor_settings() {
	EditorSettings settings;
	if (!fs::exists("editor_settings.yaml")) return settings;

	try {
		YAML::Node yaml = YAML::LoadFile("editor_settings.yaml");
		if (yaml["last_opened_scene_guid"]) settings.last_opened_scene_guid = yaml["last_opened_scene_guid"].as<std::string>();
		if (yaml["show_light_gizmos"]) settings.show_light_gizmos = yaml["show_light_gizmos"].as<bool>();
		if (yaml["light_gizmo_size"]) settings.light_gizmo_size = yaml["light_gizmo_size"].as<float>();
		if (yaml["curr_resolution"]) settings.curr_resolution = yaml["curr_resolution"].as<uint32_t>();
		if (yaml["show_skeleton_guizmos"]) settings.show_skeleton_guizmos = yaml["show_skeleton_guizmos"].as<bool>();
		if (yaml["skeleton_gizmo_size"]) settings.skeleton_gizmo_size = yaml["skeleton_gizmo_size"].as<float>();
	}
	catch (...) {
		std::cout << "failed to load editor settings" << std::endl;
	}
	return settings;
}

struct EditorLayer : Layer {
	EditorLayer();
	~EditorLayer();

	void on_attach() override;

	void on_update(float delta_time) override;

	void on_event(Event& event) override;

	bool on_key_pressed(KeyPressedEvent& event);

	bool on_mouse_pressed(MouseButtonPressedEvent& event);

	void load_scene(std::shared_ptr<Scene> const& scene = nullptr);

	void save_screenshot();

	void on_imgui_render() override;

private:
	EditorSettings m_settings;
	std::shared_ptr<EditorGUI> m_gui;
	std::shared_ptr<Scene> m_active_scene;
	std::unique_ptr<ContentBrowser> m_browser;
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
	int m_one_frame = -1;
	int m_frame_count = 0;

	void show_scene_graph();

	template<typename T, typename... Args>
	void component_context_menu(const char* label, std::shared_ptr<Entity> const& entity, Args&&... args) {
		if (entity->has_component<T>()) {
			if (ImGui::MenuItem(std::string("remove ").append(label).c_str())) {
				entity->remove_component<T>();
			}
		}
		else {
			if (ImGui::MenuItem(std::string("add ").append(label).c_str())) {
				entity->add_component<T>(std::forward<Args>(args)...);
			}
		}
	}

	void accept_payload(std::string const& data_type, std::function<void(void*)> callback);

	void show_value(void* ptr, std::type_info const& type, std::string const& name, FieldInfo const& field, const EnumInfo* enum_info = nullptr);
	void show_type_field(void* instance, FieldInfo const& field);
	void show_type_fields(void* instance, const std::string& name);

	void show_properties();;
	void show_asset_info();
	void show_settings();

	std::string get_image_info(Image* image);
	std::string get_uniform_buffer_info(UniformBuffer* buffer);

};