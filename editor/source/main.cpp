#include "iostream"
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

void save_editor_settings(EditorSettings const& settings) {
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

EditorSettings load_editor_settings() {
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
	EditorLayer() {
		m_settings = load_editor_settings();
		m_gui = std::make_shared<EditorGUI>(m_settings.curr_resolution);
		m_browser = std::make_unique<ContentBrowser>();
		m_browser->m_on_asset_opened =
			[&](AssetMeta* meta) {
				if (!meta) return;

				if (meta->type == "scene") {
					load_scene(Scene::load(meta->guid));
				}
				else {
					m_selected_asset = meta;
				}
			};

		m_gui->m_draw_viewport_overlay_func =
			[&]() {
				ImVec2 window_pos = ImVec2(
					ImGui::GetWindowPos().x + ImGui::GetWindowSize().x,
					ImGui::GetWindowPos().y + 40
				);
				ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f); // right-top pivot

				ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
				ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background

				ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // no border

				if (ImGui::Begin("overlay on viewport", nullptr,
					ImGuiWindowFlags_NoMove |
					//ImGuiWindowFlags_NoBackground |
					ImGuiWindowFlags_NoDecoration |
					ImGuiWindowFlags_AlwaysAutoResize |
					ImGuiWindowFlags_NoSavedSettings |
					ImGuiWindowFlags_NoFocusOnAppearing |
					ImGuiWindowFlags_NoNav))
				{
					ImGui::Text("overlay on viewport");
				}
				ImGui::End();

				ImGui::PopStyleColor();
			};

		m_gui->m_draw_menu_bar_items_func =
			[&]() {
				if (ImGui::BeginMenu("file")) {
					if (ImGui::MenuItem("new")) {
						load_scene();
					}
					if (ImGui::MenuItem("save")) {
						m_active_scene->save();
					}
					if (ImGui::MenuItem("exit")) {
						terminate();
					}
					ImGui::EndMenu();
				}
			};

		m_gui->m_draw_viewport_func =
			[&]() {
				auto const& cam = m_active_scene->get_main_camera();
				if (!cam) return;

				auto view = cam->get_component<CameraComponent>().get_view();
				auto proj = cam->get_component<CameraComponent>().get_proj();

				ImVec2 image_min = ImGui::GetItemRectMin(); // top-left corner
				ImVec2 image_max = ImGui::GetItemRectMax(); // bottom-right corner
				ImVec2 present_size = ImVec2(image_max.x - image_min.x, image_max.y - image_min.y);
				ImGuizmo::SetRect(image_min.x, image_min.y, present_size.x, present_size.y);

				// Visualize lights using ImGuizmo::DrawCubes
				if (m_settings.show_light_gizmos) {
					std::vector<glm::mat4> light_matrices;
					auto light_view = m_active_scene->m_registry.view<TransformComponent const, LightComponent const>();
					for (auto [entity, transform, light] : light_view.each()) {
						// Don't draw cube for selected entity to avoid clutter with the manipulator
						if (m_selected_entity && m_selected_entity->get_component<TagComponent>().m_id == m_active_scene->m_registry.get<TagComponent>(entity).m_id) {
							continue;
						}
						glm::mat4 model = transform.get_world_transform();
						model = glm::scale(model, glm::vec3(m_settings.light_gizmo_size));
						light_matrices.push_back(model);
					}

					if (!light_matrices.empty()) {
						ImGuizmo::DrawCubes(&view[0][0], &proj[0][0], (float*)light_matrices.data(), (int)light_matrices.size());
					}
				}

				// Visualize skeletons using ImGuizmo::DrawLines
				if (m_settings.show_skeleton_guizmos) {
					auto skel_view = m_active_scene->m_registry.view<TransformComponent const, AnimationComponent const, SkeletalMeshComponent const>();
					for (auto [entity, transform, anim, mesh_comp] : skel_view.each()) {
						if (anim.global_bone_transforms.empty() || !mesh_comp.m_skeleton)
							continue;

						auto const& skeleton = *mesh_comp.m_skeleton;
						std::vector<glm::mat4> matrices;
						for (auto const& global_transform : anim.global_bone_transforms) {
							glm::mat4 model = transform.get_world_transform() * global_transform;
							model = glm::scale(model, glm::vec3(m_settings.skeleton_gizmo_size));
							matrices.push_back(model);
						}
						ImGuizmo::DrawCubes(&view[0][0], &proj[0][0], (float*)matrices.data(), (int)matrices.size());

						// Draw lines connecting bones
						// We need absolute world positions for lines, not model matrices
						// ImGuizmo doesn't seem to have a simple DrawLine API that takes world coords easily mixed with DrawCubes?
						// Actually it does not. We might need to use ImGui::GetWindowDrawList() if we want 2D lines,
						// or use a debug renderer.
						// But wait, we can try to use ImGui's draw list with projected coordinates.

						// Let's implement a simple line drawer using ImGui::GetWindowDrawList()
						auto draw_list = ImGui::GetWindowDrawList();
						glm::mat4 view_proj = proj * view;

						auto world_transform = transform.get_world_transform();

						for (size_t i = 0; i < skeleton.bones.size(); ++i) {
							int parent_id = skeleton.bones[i].parent_id;
							if (parent_id != -1 && parent_id < (int)anim.global_bone_transforms.size()) {
								glm::vec3 p1 = glm::vec3(world_transform * anim.global_bone_transforms[i][3]); // Position is in last column
								glm::vec3 p2 = glm::vec3(world_transform * anim.global_bone_transforms[parent_id][3]);

								// Project to screen
								glm::vec4 clip_p1 = view_proj * glm::vec4(p1, 1.0f);
								glm::vec4 clip_p2 = view_proj * glm::vec4(p2, 1.0f);

								if (clip_p1.w > 0 && clip_p2.w > 0) { // Simple clipping check
									glm::vec3 ndc_p1 = glm::vec3(clip_p1) / clip_p1.w;
									glm::vec3 ndc_p2 = glm::vec3(clip_p2) / clip_p2.w;

									// Map to screen coords
									ImVec2 screen_p1 = {
										(ndc_p1.x + 1.0f) * 0.5f * present_size.x + image_min.x,
										(1.0f - ndc_p1.y) * 0.5f * present_size.y + image_min.y
									};
									ImVec2 screen_p2 = {
										(ndc_p2.x + 1.0f) * 0.5f * present_size.x + image_min.x,
										(1.0f - ndc_p2.y) * 0.5f * present_size.y + image_min.y
									};

									draw_list->AddLine(screen_p1, screen_p2, IM_COL32(255, 255, 0, 255), 2.0f);
								}
							}
						}
					}
				}

				if (m_selected_entity) {
						auto transform = m_selected_entity->get_component<TransformComponent>().get_world_transform();

						ImGuizmo::Manipulate(&view[0][0], &proj[0][0], m_current_gizmo_operation, m_current_gizmo_mode, &transform[0][0], NULL, NULL);

						m_is_using_gizmo = ImGuizmo::IsUsing() || ImGuizmo::IsOver();
						if (m_is_using_gizmo) {
							m_selected_entity->get_component<TransformComponent>().set_world_transform(transform);
						}
				}
			};

		// picking system does not need to be that precise
		// so we use default resolution here (512x512)
		m_picking = std::make_shared<PickingSystem>();

		if (!m_settings.last_opened_scene_guid.empty()) {
			load_scene(Scene::load(Guid::make(m_settings.last_opened_scene_guid)));
		}
		else {
			load_scene();
		}
	}

	~EditorLayer() {
		m_settings.curr_resolution = m_gui->m_current_resolution;
		save_editor_settings(m_settings);
	}

	void on_attach() override {

	}

	void on_update(float delta_time) override {
		m_fps_timer += delta_time;
		m_fps_counter += 1;
		if (m_fps_timer >= 1.0) {
			g_runtime_context.m_window->set_window_title(std::string("z1 engine fps: ") + std::to_string(m_fps_counter));
			m_fps_timer = 0.0;
			m_fps_counter = 0;
		}
		m_active_scene->on_update(delta_time);
		g_runtime_context.m_renderer_forward->draw(m_active_scene, m_gui->get_viewport_framebuffer());
		//g_runtime_context.m_renderer_2d->draw(m_active_scene, m_gui->get_viewport_framebuffer());

		g_runtime_context.m_graphics_context->bind_framebuffer(g_runtime_context.m_graphics_context->m_swapchain_framebuffer);
	}

	void on_event(Event& event) override {
		auto dispatcher = EventDispatcher(event);
		dispatcher.dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(EditorLayer::on_mouse_pressed));
		dispatcher.dispatch<KeyPressedEvent>(BIND_EVENT_FN(EditorLayer::on_key_pressed));
	}

	bool on_key_pressed(KeyPressedEvent& event) {
		switch (event.get_keycode()) {
		case KEY_W:
			m_current_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE; break;
		case KEY_E:
			m_current_gizmo_operation = ImGuizmo::OPERATION::ROTATE; break;
		case KEY_R:
			m_current_gizmo_operation = ImGuizmo::OPERATION::SCALE; break;
		default:
			break;
		}
		return false;
	}

	bool on_mouse_pressed(MouseButtonPressedEvent& event) {
		if (event.get_button() == MOUSE_BUTTON_LEFT) {
			if (m_gui->is_viewport_focused() && m_gui->is_viewport_hovered() && !m_is_using_gizmo) {
				auto start = std::chrono::high_resolution_clock::now();

				m_picking->render(m_active_scene);

				float x = 0.0f;
				float y = 0.0f;
				m_gui->get_mouse_cursor_on_viewport(&x, &y);

				auto object_id = m_picking->query(x, y);
				if (object_id == INVALID_INDEX) {
					m_selected_entity = nullptr;
				}
				else {
					m_selected_entity = m_active_scene->m_entities[object_id];
					m_picked_from_viewport = true;
				}

				auto end = std::chrono::high_resolution_clock::now();
				auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
					end - start
				).count();
				std::cout << "picking object (ms): " << ms << "\n";
			}
		}
		return false;
	}

	void load_scene(std::shared_ptr<Scene> const& scene = nullptr) {
		if (scene) {
			m_active_scene = scene;
			m_settings.last_opened_scene_guid = m_active_scene->m_meta.guid.value;
		}
		else {
			auto folder = m_browser->get_curr_dir();
			m_active_scene = Scene::create(folder / "new_scene");
			if (!m_active_scene) {
				return;
			}
			m_settings.last_opened_scene_guid = m_active_scene->m_meta.guid.value;
		}

		m_selected_entity = nullptr;

		// setup editor viewport camera
		auto camera = m_active_scene->create_transient_entity("[Editor] Viewport Camera");
		auto& cam_comp = camera->add_component<CameraComponent>();
		auto& trans_comp = camera->get_component<TransformComponent>();

		if (m_active_scene->m_editor_camera_data.is_valid) {
			cam_comp.m_aspect = m_active_scene->m_editor_camera_data.camera.m_aspect;
			cam_comp.m_near = m_active_scene->m_editor_camera_data.camera.m_near;
			cam_comp.m_far = m_active_scene->m_editor_camera_data.camera.m_far;
			cam_comp.m_use_fixed_aspect = m_active_scene->m_editor_camera_data.camera.m_use_fixed_aspect;
			cam_comp.m_intrinsic = m_active_scene->m_editor_camera_data.camera.m_intrinsic;

			trans_comp.m_location = m_active_scene->m_editor_camera_data.transform.m_location;
			trans_comp.m_rotation = m_active_scene->m_editor_camera_data.transform.m_rotation;
			trans_comp.m_scale = m_active_scene->m_editor_camera_data.transform.m_scale;
		}
		else {
			trans_comp.m_location = { 0.0f, 0.0f, 5.0f };
		}

		camera->attach_script<HoveringCameraCtrlScript>(m_gui);
		m_active_scene->set_main_camera(camera);
	}

	void on_imgui_render() override {
		m_gui->draw();

		if (ImGui::Begin("debug")) {
			if (ImGui::RadioButton("v sync", g_runtime_context.m_window->is_v_sync_enabled())) {
				g_runtime_context.m_window->set_v_sync(!g_runtime_context.m_window->is_v_sync_enabled());
			}

			ImGui::Text(std::string("viewport_pixel_scale_x: " + std::to_string(m_gui->m_viewport_pixel_scale_x)).c_str());

			if (ImGui::Button("save default.ini")) {
				ImGui::SaveIniSettingsToDisk("editor/default.ini");
			}

			if (ImGui::Button("save screenshot")) {
				auto& fb = m_gui->get_viewport_framebuffer();
				uint32_t width = fb->get_width();
				uint32_t height = fb->get_height();

				// Allocate buffer for RGBA8 pixels
				std::vector<unsigned char> pixels(width * height * 4);

				fb->read_pixels(0, 0, 0, width, height, pixels.data());

				// *** OPTIONAL: flip vertically for correct orientation ***
				std::vector<unsigned char> flipped(width * height * 4);
				for (uint32_t y = 0; y < height; ++y) {
					std::memcpy(
						&flipped[y * width * 4],
						&pixels[(height - 1 - y) * width * 4],
						width * 4
					);
				}

				std::string filename = "screenshot.png";

				// Save using stb_image_write (PNG)
				if (!stbi_write_png(filename.c_str(), width, height, 4, flipped.data(), width * 4)) {
					std::cerr << "Failed to write image!" << std::endl;
				}
				else {
					std::cout << "Saved: " << filename << std::endl;
				}
			}

			ImGui::DragFloat("light gizmo size", &m_settings.light_gizmo_size, 0.01f, 0.0f, 1.0f);
			ImGui::Checkbox("show light gizmos", &m_settings.show_light_gizmos);
			ImGui::DragFloat("skeleton gizmo size", &m_settings.skeleton_gizmo_size, 0.01f, 0.0f, 1.0f);
			ImGui::Checkbox("show skeleton gizmos", &m_settings.show_skeleton_guizmos);

		}
		ImGui::End();

		show_asset_info();
		show_scene_graph();
		show_properties();
		show_settings();
		m_browser->draw();
	}

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

	void show_scene_graph() {
		if (ImGui::Begin("scene")) {
			ImGui::Text("entities in scene: %llu", m_active_scene->get_entity_count());
			ImGui::Separator();

			std::unordered_map<TransformComponent*, Entity*> transform_to_entity;
			std::unordered_map<Entity*, std::vector<std::shared_ptr<Entity>>> children;
			std::vector<std::shared_ptr<Entity>> roots;

			// 1. Map transforms to entities
			for (auto const& ent : m_active_scene->m_entities) {
				if (ent) {
					transform_to_entity[&ent->get_component<TransformComponent>()] = ent.get();
				}
			}

			// 2. Build hierarchy
			for (auto const& ent : m_active_scene->m_entities) {
				if (!ent) continue;
				auto& tc = ent->get_component<TransformComponent>();
				if (tc.m_parent) {
					auto it = transform_to_entity.find(tc.m_parent);
					if (it != transform_to_entity.end()) {
						children[it->second].push_back(ent);
					}
					else {
						roots.push_back(ent); // parent not found in scene entities
					}
				}
				else {
					roots.push_back(ent);
				}
			}

			// 3. Recursive draw function
			std::function<void(std::shared_ptr<Entity>)> draw_node;
			draw_node = [&](std::shared_ptr<Entity> entity) {
				if (!entity) return;
				auto const& tag = entity->get_component<TagComponent>();

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
				if (children[entity.get()].empty()) {
					flags |= ImGuiTreeNodeFlags_Leaf;
				}
				if (m_selected_entity == entity) {
					flags |= ImGuiTreeNodeFlags_Selected;
				}

				bool opened = ImGui::TreeNodeEx((void*)(intptr_t)tag.m_id, flags, "%s", tag.m_tag.c_str());

				if (ImGui::IsItemClicked()) {
					m_selected_entity = entity;
				}

				// Drag Source
				if (ImGui::BeginDragDropSource()) {
					Entity* ptr = entity.get();
					ImGui::SetDragDropPayload("ENTITY_ITEM", &ptr, sizeof(Entity*));
					ImGui::Text("%s", tag.m_tag.c_str());
					ImGui::EndDragDropSource();
				}

				// Drop Target (Reparenting)
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ITEM")) {
						Entity* source_ptr = *(Entity**)payload->Data;
						// Find shared_ptr for source
						std::shared_ptr<Entity> source_entity;
						for (auto& e : m_active_scene->m_entities) {
							if (e.get() == source_ptr) {
								source_entity = e;
								break;
							}
						}

						if (source_entity && source_entity != entity) {
							// Check for circular dependency
							bool circular = false;
							Entity* check = entity.get();
							while (check) {
								if (check == source_entity.get()) {
									circular = true;
									break;
								}
								auto& tc = check->get_component<TransformComponent>();
								if (tc.m_parent) {
									auto it = transform_to_entity.find(tc.m_parent);
									check = (it != transform_to_entity.end()) ? it->second : nullptr;
								}
								else {
									check = nullptr;
								}
							}

							if (!circular) {
								source_entity->get_component<TransformComponent>().set_parent(&entity->get_component<TransformComponent>());
							}
						}
					}

					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_ITEM")) {
						AssetMeta* meta = *(AssetMeta**)payload->Data;
						if (meta && meta->type == "prefab") {
							auto prefab = Prefab::load(meta->guid);
							if (prefab) {
								auto new_entities = prefab->instantiate(m_active_scene);
								for (auto& new_entity : new_entities) {
									new_entity->get_component<TransformComponent>().set_parent(&entity->get_component<TransformComponent>());
								}
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				// Context Menu
				if (ImGui::BeginPopupContextItem()) {
					if (ImGui::MenuItem("Delete")) {
						if (m_selected_entity == entity) m_selected_entity = nullptr;
						m_active_scene->destroy_entity(entity);
					}
					ImGui::EndPopup();
				}

				if (opened) {
					for (auto& child : children[entity.get()]) {
						draw_node(child);
					}
					ImGui::TreePop();
				}
			};

			// 4. Draw roots
			for (auto& root : roots) {
				draw_node(root);
			}

			// Drop target for background (instantiate as root)
			ImGui::Dummy(ImGui::GetContentRegionAvail());
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_ITEM")) {
					AssetMeta* meta = *(AssetMeta**)payload->Data;
					if (meta && meta->type == "prefab") {
						auto prefab = Prefab::load(meta->guid);
						if (prefab) {
							prefab->instantiate(m_active_scene);
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			// Right click on empty space to create entity
			if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
				if (ImGui::MenuItem("create empty entity")) {
					m_active_scene->create_entity("new entity");
				}
				ImGui::EndPopup();
			}

			// Drop target on window background to detach (make root)
			// This is tricky with ImGui child windows.
			// A simple way is to check if mouse is in window and payload is active, but not hovering any item.
			// But that logic is complex to get right without 'BeginDragDropTargetCustom'.
			// We'll skip "detach via drag to background" for now unless explicitly requested.
			// Users can detach by dragging to "Scene" header if we added a target there, or we can add a "Detach" context menu.
		}
		ImGui::End();
	}

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

	void accept_payload(std::string const& data_type, std::function<void(void*)> callback) {
		auto payload = ImGui::GetDragDropPayload();
		if (payload && payload->IsDataType("ASSET_ITEM")) {

			ImVec2 min = ImGui::GetItemRectMin(); // top-left of last item
			ImVec2 max = ImGui::GetItemRectMax(); // bottom-right of last item

			ImDrawList* draw_list = ImGui::GetWindowDrawList();

			// Border thickness and color
			float thickness = 2.0f;
			ImU32 color = IM_COL32(255, 0, 0, 255); // red

			draw_list->AddRect(min, max, color, 0.0f, 0, thickness);

			if (ImGui::BeginDragDropTarget()) {
				if (ImGui::AcceptDragDropPayload("ASSET_ITEM")) {
					callback(payload->Data);
				}
				ImGui::EndDragDropTarget();
			}
		}
	}

#define SHOW_FLOAT_FIELD(num, value)                                      \
	float min = field.get_widget_value<float>("min", -1e6f);              \
	float max = field.get_widget_value<float>("max",  1e6f);              \
	float step = field.get_widget_value<float>("step", 0.01f);            \
	if (field.is_widget_type("slider")) {                                 \
		ImGui::SliderFloat##num(field.name.c_str(), value, min, max);     \
	}                                                                     \
	else if (field.is_widget_type("drag")) {                              \
		ImGui::DragFloat##num(field.name.c_str(), value, step, min, max); \
	}                                                                     \
	else {                                                                \
		ImGui::InputFloat##num(field.name.c_str(), value);                \
	}

#define SHOW_FLOAT_FIELD_WITH_COLOR(num, value)                           \
	float min = field.get_widget_value<float>("min", -1e6f);              \
	float max = field.get_widget_value<float>("max",  1e6f);              \
	float step = field.get_widget_value<float>("step", 0.01f);            \
	if (field.is_widget_type("color")) {                                  \
		ImGui::ColorEdit##num(field.name.c_str(), value);                 \
	}                                                                     \
	else if (field.is_widget_type("slider")) {                            \
		ImGui::SliderFloat##num(field.name.c_str(), value, min, max);     \
	}                                                                     \
	else if (field.is_widget_type("drag")) {                              \
		ImGui::DragFloat##num(field.name.c_str(), value, step, min, max); \
	}                                                                     \
	else {                                                                \
		ImGui::InputFloat##num(field.name.c_str(), value);                \
	}

#define ACCEPT_PAYLOAD(asset_type, meta_type)                             \
	accept_payload("ASSET_ITEM", [&](void* data) {                        \
		AssetMeta* meta = *(AssetMeta**)data;                             \
		if (meta->type == meta_type) {                                    \
			value = asset_type::load(meta->guid);                         \
		}                                                                 \
	});

	void show_type_field(void* instance, FieldInfo const& field) {
		bool const visible = (field.flag & FF_Visible) != 0;
		bool const editable = (field.flag & FF_Editable) != 0;
		if (!visible && !editable)
			return;

		if (!editable)
			ImGui::BeginDisabled();

		if (*field.type == typeid(bool)) {
			bool& value = field.get<bool>(instance);
			if (field.is_widget_type("radio")) {
				if (ImGui::RadioButton(field.name.c_str(), value)) {
					value = !value;
				}
			}
			else {
				ImGui::Checkbox(field.name.c_str(), &value);
			}
		}
		else if (*field.type == typeid(float)) {
			float& value = field.get<float>(instance);
			SHOW_FLOAT_FIELD(, &value)
		}
		else if (*field.type == typeid(int)) {
			int& value = field.get<int>(instance);
			int min = field.get_widget_value<int>("min", -10000);
			int max = field.get_widget_value<int>("max", 10000);
			int step = field.get_widget_value<int>("step", 1);
			if (field.is_widget_type("slider")) {
				ImGui::SliderInt(field.name.c_str(), &value, min, max);
			}
		else if (field.is_widget_type("drag")) {
				ImGui::DragInt(field.name.c_str(), &value, (float)step, min, max);
			}
			else {
				ImGui::InputInt(field.name.c_str(), &value, step);
			}
		}
		else if (*field.type == typeid(glm::vec2)) {
			glm::vec2& value = field.get<glm::vec2>(instance);
			SHOW_FLOAT_FIELD(2, &value[0])
		}
		else if (*field.type == typeid(glm::vec3)) {
			glm::vec3& value = field.get<glm::vec3>(instance);
			SHOW_FLOAT_FIELD_WITH_COLOR(3, &value[0])
		}
		else if (*field.type == typeid(glm::vec4)) {
			glm::vec4& value = field.get<glm::vec4>(instance);
			SHOW_FLOAT_FIELD_WITH_COLOR(4, &value[0])
		}
		else if (*field.type == typeid(std::string)) {
			std::string& value = field.get<std::string>(instance);
			static char str_buffer[256] = {};
			strcpy_s(str_buffer, value.c_str());
			if (ImGui::InputText(field.name.c_str(), str_buffer, IM_ARRAYSIZE(str_buffer))) {
				value = std::string(str_buffer);
			}
		}
		else if (*field.type == typeid(std::shared_ptr<Texture2D>)) {
			std::shared_ptr<Texture2D>& value = field.get<std::shared_ptr<Texture2D>>(instance);
			ImGui::Text(field.name.c_str());
			ImGui::Indent();
			if (value) {
				auto w = value->m_image->get_description().m_width;
				auto h = value->m_image->get_description().m_height;
				ImGui::Image(value->m_image->get_native_handle(), ImVec2(64.0f * w / h, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
			}
			else {
				ImGui::Text("No Texture");
			}
			ACCEPT_PAYLOAD(Texture2D, "texture2d")
			ImGui::Unindent();
		}
		else if (*field.type == typeid(std::shared_ptr<Animation>)) {
			std::shared_ptr<Animation>& value = field.get<std::shared_ptr<Animation>>(instance);
			ImGui::Text(field.name.c_str());
			ImGui::Indent();
			if (value) {
				ImGui::Text("guid: %s", value->m_meta.guid.value.c_str());
				ImGui::Text("name: %s", value->name.c_str());
				ImGui::Text("duration: %.2fs", value->duration);
				ImGui::Text("ticks per second: %.2f", value->ticks_per_second);
			}
			else {
				ImGui::Text("No Animation");
			}
			ACCEPT_PAYLOAD(Animation, "animation")
			ImGui::Unindent();
		}

		if (!editable)
			ImGui::EndDisabled();
	}

	void show_type_fields(void* instance, const std::string& name) {
		auto const info = TypeRegistry::instance().get(name);
		if (!info) return;

		ImGui::Indent();
		for (auto& field : info->fields) {
			show_type_field(instance, field);
		}
		ImGui::Unindent();
	}

#define SHOW_COMPONENT(ComponentType)                                               \
	if (ImGui::CollapsingHeader(#ComponentType, ImGuiTreeNodeFlags_DefaultOpen)) {  \
		auto& comp = m_selected_entity->get_component<ComponentType>();             \
		show_type_fields(&comp, TYPE_NAME(ComponentType));                          \
	}

	void show_properties() {
		if (ImGui::Begin("properties")) {
			if (m_selected_entity) {
				SHOW_COMPONENT(TagComponent)
				SHOW_COMPONENT(TransformComponent)
				if (m_selected_entity->has_component<SkyLightComponent>()) {
					SHOW_COMPONENT(SkyLightComponent)
				}
				if (m_selected_entity->has_component<AnimationComponent>()) {
					SHOW_COMPONENT(AnimationComponent)
				}
				if (m_selected_entity->has_component<LightComponent>()) {
					SHOW_COMPONENT(LightComponent)
				}

				if (m_selected_entity->has_component<SpriteComponent>()) {
					if (ImGui::CollapsingHeader("SpriteComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
						auto& sprite = m_selected_entity->get_component<SpriteComponent>();
						show_type_fields(&sprite, "SpriteComponent");

						ImGui::Text("texture");
						ImGui::Indent();

						if (sprite.m_texture) {
							auto w = sprite.m_texture->m_image->get_description().m_width;
							auto h = sprite.m_texture->m_image->get_description().m_height;
							ImGui::Image(sprite.m_texture->m_image->get_native_handle(), ImVec2(64.0f * w / h, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
						}
						else {
							ImGui::Text("No Texture");
						}
						accept_payload("ASSET_ITEM",
							[&](void* data) {
								AssetMeta* meta = *(AssetMeta**)data;
								if (meta->type == "texture2d") {
									sprite.m_texture = Texture2D::load(meta->guid);
								}
							}
						);

						if (sprite.m_texture) {
							auto w = sprite.m_texture->m_image->get_description().m_width;
							auto h = sprite.m_texture->m_image->get_description().m_height;
							ImGui::Text("guid: %s", sprite.m_texture->m_meta.guid.value.c_str());
							ImGui::Text("width: %d", w);
							ImGui::Text("height: %d", h);
							ImGui::Text("depth: %d", sprite.m_texture->m_image->get_description().m_depth);
							ImGui::Text("format: %s", get_image_format_name(sprite.m_texture->m_image->get_description().m_format).c_str());
							ImGui::Text("sampler Mode: %s", get_sampler_mode_name(sprite.m_texture->m_image->get_description().m_sampler_mode).c_str());
							ImGui::Text("wrap Mode: %s", get_wrap_mode_name(sprite.m_texture->m_image->get_description().m_wrap_mode).c_str());
							ImGui::RadioButton("mipmap", sprite.m_texture->m_image->get_description().m_mipmap);
						}
						ImGui::Unindent();

						ImGui::Text("texcoords");
						ImGui::Indent();
						for (int i = 0; i < 4; ++i) {
							ImGui::DragFloat2(("texcoord " + std::to_string(i)).c_str(), &sprite.m_texcoords[i][0], 0.01f);
						}
						ImGui::Unindent();
					}

				}

				if (m_selected_entity->has_component<StaticMeshComponent>()) {
					if (ImGui::CollapsingHeader("StaticMeshComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
						auto& mesh = m_selected_entity->get_component<StaticMeshComponent>();
						ImGui::Text("guid: %s", mesh.m_mesh->m_meta.guid.value.c_str());
						ImGui::Text("primitives");
						ImGui::Indent();
						for (int i = 0; i < mesh.m_mesh->m_primitives.size(); ++i) {
							auto const& prim = mesh.m_mesh->m_primitives[i];
							if (ImGui::CollapsingHeader(("primitive " + std::to_string(i)).c_str())) {
								ImGui::Text("triangle count: %d", prim.get_triangle_count());
								ImGui::Text("material");
								ImGui::Indent();
								if (prim.m_material.is_valid()) {
									auto mat_meta = g_runtime_context.m_asset_manager->get_meta(prim.m_material);
									ImGui::Text("guid: %s", mat_meta.guid.value.c_str());
									ImGui::Text("path: %s", mat_meta.path.generic_string().c_str());
								}
								else {
									ImGui::Text("no material attached");
								}
								accept_payload("ASSET_ITEM",
									[&](void* data) {
										AssetMeta* meta = *(AssetMeta**)data;
										if (meta->type == "material" || meta->type == "material instance") {
											mesh.m_mesh->m_primitives[i].m_material = meta->guid;
										}
									}
								);
								ImGui::Unindent();
							}
						}
						ImGui::Unindent();
					}
				}

				if (m_selected_entity->has_component<SkeletalMeshComponent>()) {
					if (ImGui::CollapsingHeader("SkeletalMeshComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
						auto& mesh = m_selected_entity->get_component<SkeletalMeshComponent>();
						ImGui::Text("guid: %s", mesh.m_mesh->m_meta.guid.value.c_str());
						ImGui::Text("primitives");
						ImGui::Indent();
						for (int i = 0; i < mesh.m_mesh->m_primitives.size(); ++i) {
							auto const& prim = mesh.m_mesh->m_primitives[i];
							if (ImGui::CollapsingHeader(("primitive " + std::to_string(i)).c_str())) {
								ImGui::Text("triangle count: %d", prim.get_triangle_count());
								ImGui::Text("material");
								ImGui::Indent();
								if (prim.m_material.is_valid()) {
									auto mat_meta = g_runtime_context.m_asset_manager->get_meta(prim.m_material);
									ImGui::Text("guid: %s", mat_meta.guid.value.c_str());
									ImGui::Text("path: %s", mat_meta.path.generic_string().c_str());
								} else {
									ImGui::Text("no material attached");
								}
								accept_payload("ASSET_ITEM",
									[&](void* data) {
										AssetMeta* meta = *(AssetMeta**)data;
										if (meta->type == "material" || meta->type == "material instance") {
											mesh.m_mesh->m_primitives[i].m_material = meta->guid;
										}
									}
								);
								ImGui::Unindent();
							}
						}
						ImGui::Unindent();

						if (mesh.m_skeleton) {
							ImGui::Text("skeleton guid: %s", mesh.m_skeleton->m_meta.guid.value.c_str());
							ImGui::Text("joint count: %d", mesh.m_skeleton->bones.size());
						}
						else {
							ImGui::Text("no skeleton attached");
						}

						accept_payload("ASSET_ITEM", [&](void* data) {
							AssetMeta* meta = *(AssetMeta**)data;
							if (meta->type == "skeleton") {
								mesh.m_skeleton = Skeleton::load(meta->guid);
							}
						});
					}
				}

				if (m_selected_entity->has_component<CameraComponent>()) {
					if (ImGui::CollapsingHeader("CameraComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
						auto& camera = m_selected_entity->get_component<CameraComponent>();
						show_type_fields(&camera, "CameraComponent");

						if (camera.m_is_perspective) {
							ImGui::InputFloat("field of view", &camera.m_intrinsic.fov, 0.01f);
						}
						else {
							ImGui::InputFloat("frustum size", &camera.m_intrinsic.size, 0.01f);
						}
						if (ImGui::RadioButton("is primary", camera.m_is_primary)) {
							if (!camera.m_is_primary) {
								m_active_scene->set_main_camera(m_selected_entity);
							}
						}
					}
				}

				if (m_selected_entity->has_component<ScriptComponent>()) {
					if (ImGui::CollapsingHeader("ScriptComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
						auto& script = m_selected_entity->get_component<ScriptComponent>();
						bool new_script_added = false;
						if (ImGui::BeginCombo("add", "select script...")) {
							if (ImGui::Selectable("Generic2DCameraCtrlScript")) {
								m_selected_entity->attach_script<Generic2DCameraCtrlScript>(m_gui);
								new_script_added = true;
							}
							if (ImGui::Selectable("HoveringCameraCtrlScript")) {
								m_selected_entity->attach_script<HoveringCameraCtrlScript>(m_gui);
								new_script_added = true;
							}
							ImGui::EndCombo();
						}
						if (!new_script_added) {
							ImGui::Text("attached:");
							ImGui::Separator();
							size_t to_remove = INVALID_INDEX;
							for (size_t i = 0; i < script.m_scripts.size(); ++i) {
								auto const& sd = script.m_scripts[i];
								ImGui::Text(sd.instance->get_script_name().c_str());
								ImGui::SameLine();
								if (ImGui::Button("remove")) {
									to_remove = i;
								}
							}
							if (to_remove != INVALID_INDEX) {
								script.unbind_at(to_remove);
							}
						}
					}
				}

				if (ImGui::BeginPopupContextWindow()) {
					component_context_menu<CameraComponent>("camera component", m_selected_entity);
					component_context_menu<LightComponent>("light component", m_selected_entity);
					component_context_menu<SpriteComponent>("sprite component", m_selected_entity);
					component_context_menu<ScriptComponent>("script component", m_selected_entity, m_selected_entity);
					component_context_menu<SkyLightComponent>("skylight component", m_selected_entity);
					component_context_menu<AnimationComponent>("animation component", m_selected_entity);
					ImGui::EndPopup();
				}
			}
		}
		ImGui::End();
	}

	void show_asset_info() {
		if (ImGui::Begin("asset viewer")) {
			if (!m_selected_asset) {
				ImGui::Text("no asset selected");
				ImGui::End();
				return;
			}

			ImGui::Text("guid: %s", m_selected_asset->guid.value.c_str());
			ImGui::Text("type: %s", m_selected_asset->type.c_str());
			ImGui::Text("path: %s", m_selected_asset->path.generic_string().c_str());
			ImGui::Separator();

			if (m_selected_asset->type == "shader") {
				auto shader = g_runtime_context.m_asset_manager->get<Shader>(m_selected_asset->guid);
				ImGui::Text("basic info");
				if (ImGui::BeginTable("basic info", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
					ImGui::TableNextColumn(); ImGui::Text("name"); ImGui::TableNextColumn(); ImGui::Text(shader->get_name().c_str());
					ImGui::TableNextColumn(); ImGui::Text("path"); ImGui::TableNextColumn(); ImGui::Text(shader->get_path().c_str());
					ImGui::TableNextColumn(); ImGui::Text("guid"); ImGui::TableNextColumn(); ImGui::Text(shader->m_guid.value.c_str());
					ImGui::EndTable();
				}
				ImGui::Text("shader attributes");
				if (ImGui::BeginTable("shader attributes", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
					ImGui::TableSetupColumn("name");
					ImGui::TableSetupColumn("type");
					ImGui::TableSetupColumn("location");
					ImGui::TableSetupColumn("count");
					ImGui::TableHeadersRow();

					for (auto& attrib : shader->get_attributes()) {
						ImGui::TableNextColumn(); ImGui::Text(attrib.m_name.c_str());
						ImGui::TableNextColumn(); ImGui::Text((get_data_type_name(attrib.m_type)).c_str());
						ImGui::TableNextColumn(); ImGui::Text(std::to_string(attrib.m_location).c_str());
						ImGui::TableNextColumn(); ImGui::Text(std::to_string(attrib.m_count).c_str());
					}
					ImGui::EndTable();
				}
				ImGui::Text("shader uniforms");
				if (ImGui::BeginTable("shader uniforms", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
					ImGui::TableSetupColumn("name");
					ImGui::TableSetupColumn("type");
					ImGui::TableSetupColumn("location");
					ImGui::TableSetupColumn("count");
					ImGui::TableHeadersRow();

					ImGui::TableNextRow();
					for (auto& uniform : shader->get_uniforms()) {
						if (uniform.m_location == INVALID_LOCATION) continue;
						ImGui::TableNextColumn(); ImGui::Text(uniform.m_name.c_str());
						ImGui::TableNextColumn(); ImGui::Text((get_data_type_name(uniform.m_type)).c_str());
						ImGui::TableNextColumn(); ImGui::Text(std::to_string(uniform.m_location).c_str());
						ImGui::TableNextColumn(); ImGui::Text(std::to_string(uniform.m_count).c_str());
					}
					ImGui::EndTable();
				}
				ImGui::Text("shader uniform blocks");
				for (auto& block : shader->get_uniform_blocks()) {
					if (ImGui::BeginTable("shader uniform blocks", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
						ImGui::TableNextColumn(); ImGui::Text("name"); ImGui::TableNextColumn(); ImGui::Text(block.m_name.c_str());
						ImGui::TableNextColumn(); ImGui::Text("size"); ImGui::TableNextColumn(); ImGui::Text(std::to_string(block.m_size).c_str());
						ImGui::TableNextColumn(); ImGui::Text("binding"); ImGui::TableNextColumn(); ImGui::Text(std::to_string(block.m_binding).c_str());

						ImGui::TableNextColumn(); ImGui::Text("variables"); ImGui::TableNextColumn();
						if (ImGui::BeginTable("variables", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
							ImGui::TableSetupColumn("name");
							ImGui::TableSetupColumn("type");
							ImGui::TableSetupColumn("count");
							ImGui::TableHeadersRow();

							for (auto& variable : block.m_variables) {
								ImGui::TableNextColumn(); ImGui::Text(variable.m_name.c_str());
								ImGui::TableNextColumn(); ImGui::Text((get_data_type_name(variable.m_type)).c_str());
								ImGui::TableNextColumn(); ImGui::Text(std::to_string(variable.m_count).c_str());
							}
							ImGui::EndTable();
						}
						ImGui::EndTable();
					}
				}
			}
			else if (m_selected_asset->type == "material") {
				auto mat = g_runtime_context.m_asset_manager->get<Material>(m_selected_asset->guid);
				ImGui::Text("shader name: %s", mat->m_pipeline->m_shader->get_name().c_str());

				ImGui::Text("variables");
				for (auto& [name, var] : mat->m_variables) {
					if (!var.visible) continue;

					ImGui::BeginDisabled();
					switch (var.type) {
					case DataType::Float: ImGui::InputFloat(("##" + name).c_str(), var.default_value.vec); break;
					case DataType::Float2: ImGui::InputFloat2(("##" + name).c_str(), var.default_value.vec); break;
					case DataType::Float3: ImGui::InputFloat3(("##" + name).c_str(), var.default_value.vec); break;
					case DataType::Float4: ImGui::InputFloat4(("##" + name).c_str(), var.default_value.vec); break;
					case DataType::Int: ImGui::InputInt(("##" + name).c_str(), var.default_value.ivec); break;
					case DataType::Int2: ImGui::InputInt2(("##" + name).c_str(), var.default_value.ivec); break;
					case DataType::Int3: ImGui::InputInt3(("##" + name).c_str(), var.default_value.ivec); break;
					case DataType::Int4: ImGui::InputInt4(("##" + name).c_str(), var.default_value.ivec); break;
					case DataType::Sampler2D: ImGui::Text(name.c_str()); break;
					case DataType::Sampler2DArray: ImGui::Text(name.c_str()); break;
					case DataType::SamplerCube: ImGui::Text(name.c_str()); break;
					}

					ImGui::EndDisabled();

					if (var.type == DataType::Sampler2D && var.default_value.tex2D) {
						ImGui::SameLine();
						auto const& texture = var.default_value.tex2D;
						auto w = texture->m_image->get_description().m_width;
						auto h = texture->m_image->get_description().m_height;
						ImGui::Image(texture->m_image->get_native_handle(), ImVec2(64.0f * w / h, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
					}
				}
			}
			else if (m_selected_asset->type == "material instance") {
				auto mi = g_runtime_context.m_asset_manager->get<MaterialInstance>(m_selected_asset->guid);
				ImGui::Text("parent material: %s", mi->m_material->m_meta.path.generic_string().c_str());
				ImGui::Text("shader name: %s", mi->m_material->m_pipeline->m_shader->get_name().c_str());

				ImGui::Text("override variables");
				for (auto& [name, var] : mi->m_override_variables) {
					if (!var.visible) continue;

					ImGui::Checkbox(name.c_str(), &var.default_value.valid);
					ImGui::SameLine();
					if (!var.default_value.valid) {
						ImGui::BeginDisabled();
					}
					switch (var.type) {
					case DataType::Float: ImGui::InputFloat(("##" + name).c_str(), var.default_value.vec); break;
					case DataType::Float2: ImGui::InputFloat2(("##" + name).c_str(), var.default_value.vec); break;
					case DataType::Float3: ImGui::InputFloat3(("##" + name).c_str(), var.default_value.vec); break;
					case DataType::Float4: ImGui::InputFloat4(("##" + name).c_str(), var.default_value.vec); break;
					case DataType::Int: ImGui::InputInt(("##" + name).c_str(), var.default_value.ivec); break;
					case DataType::Int2: ImGui::InputInt2(("##" + name).c_str(), var.default_value.ivec); break;
					case DataType::Int3: ImGui::InputInt3(("##" + name).c_str(), var.default_value.ivec); break;
					case DataType::Int4: ImGui::InputInt4(("##" + name).c_str(), var.default_value.ivec); break;
					}

					if (!var.default_value.valid) {
						ImGui::EndDisabled();
					}

					if (var.type == DataType::Sampler2D && var.default_value.tex2D) {
						auto const& texture = var.default_value.tex2D;
						auto w = texture->m_image->get_description().m_width;
						auto h = texture->m_image->get_description().m_height;
						ImGui::Image(texture->m_image->get_native_handle(), ImVec2(64.0f * w / h, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
						accept_payload("ASSET_ITEM",
							[&](void* data) {
								AssetMeta* meta = *(AssetMeta**)data;
								if (meta->type == "texture2d") {
									var.default_value.tex2D = Texture2D::load(meta->guid);
								}
							}
						);
					}
				}
			}
			else if (m_selected_asset->type == "texture2d") {
				auto tex = g_runtime_context.m_asset_manager->get<Texture2D>(m_selected_asset->guid);
				auto region = ImGui::GetContentRegionAvail();
				auto w = tex->m_image->get_description().m_width;
				auto h = tex->m_image->get_description().m_height;
				ImGui::Image(tex->m_image->get_native_handle(), ImVec2(region.x, region.x * h / w), ImVec2(0, 1), ImVec2(1, 0));
			}
			else if (m_selected_asset->type == "static mesh") {
				if (ImGui::Button("add to scene")) {
					auto ent = m_active_scene->create_entity(m_selected_asset->name());
					ent->add_component<StaticMeshComponent>(
						g_runtime_context.m_asset_manager->get<StaticMesh>(m_selected_asset->guid)
					);
				}
			}
			else if (m_selected_asset->type == "skeletal mesh") {
				if (ImGui::Button("add to scene")) {
					auto ent = m_active_scene->create_entity(m_selected_asset->name());
					ent->add_component<SkeletalMeshComponent>(
						g_runtime_context.m_asset_manager->get<SkeletalMesh>(m_selected_asset->guid));
				}
			}
			else if (m_selected_asset->type == "prefab") {
				if (ImGui::Button("instantiate in scene")) {
					auto prefab = Prefab::load(m_selected_asset->guid);
					if (prefab) {
						prefab->instantiate(m_active_scene);
					}
				}
			}
			else if (m_selected_asset->type == "animation") {
				auto anim = g_runtime_context.m_asset_manager->get<Animation>(m_selected_asset->guid);
				ImGui::Text("duration: %.2fs", anim->duration);
				ImGui::Text("ticks per second: %.2f", anim->ticks_per_second);
				ImGui::Text("channels: %d", anim->channels.size());
				for (int c = 0; c < anim->channels.size(); ++c) {
					auto& channel = anim->channels[c];
					if (ImGui::CollapsingHeader(("channel " + std::to_string(c)).c_str())) {
						ImGui::Text("bone id: %d", channel.bone_id);
						ImGui::Text("bone name: %s", channel.bone_name.c_str());
						ImGui::Text("position frame count: %d", channel.position_keys.size());
						ImGui::Text("rotation frame count: %d", channel.rotation_keys.size());
						ImGui::Text("scale frame count: %d", channel.scale_keys.size());
					}
				}
			}
		}
		ImGui::End();
	}

	void show_settings() {
		if (ImGui::Begin("settings")) {
			show_type_fields(g_runtime_context.m_global.get(), TYPE_NAME(GlobalSettings));
		}
		ImGui::End();
	}

	std::string get_image_info(Image* image) {
		auto const& desc = image->get_description();
		std::string info = "";

		info += std::to_string(desc.m_width) + "x" + std::to_string(desc.m_height) + " ";
		info += get_image_format_name(desc.m_format) + " ";
		info += get_sampler_mode_name(desc.m_sampler_mode) + " ";
		info += get_wrap_mode_name(desc.m_wrap_mode);

		return info;
	}

	std::string get_uniform_buffer_info(UniformBuffer* buffer) {
		std::string info = "size: ";

		info += std::to_string(buffer->get_size()) + " byte";

		return info;
	}
};

struct EditorApp : Application {
	void init() override {
		fs::path ini_path = "imgui.ini";
		if (!fs::exists(ini_path)) {
			fs::path default_path = "editor/default.ini";
			if (fs::exists(default_path)) {
				fs::copy_file(default_path, ini_path);
			}
		}

		push_layer(std::make_shared<EditorLayer>());
	};
};

int main() {
	auto start = std::chrono::high_resolution_clock::now();
	std::cout << "hello world!\n";

	EditorApp app;
	app.init();

	auto end = std::chrono::high_resolution_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		end - start
	).count();

	std::cout << "app launch (ms): " << ms << "\n";

	app.run();

	return 0;
}
