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

using namespace z1;
namespace fs = std::filesystem;

struct EditorLayer : Layer {
	EditorLayer() {
		m_gui = std::make_shared<EditorGUI>();
		m_active_scene = std::make_shared<Scene>();
		m_active_scene->m_main_framebuffer = m_gui->get_viewport_framebuffer();
		m_browser = std::make_unique<ContentBrowser>();
		m_browser->m_on_asset_opened =
			[&](AssetMeta* meta) {
				if (!meta) return;
				if (meta->root == "engine") return;

				if (meta->type == "scene") {
					m_active_scene = Scene::deserialize(meta->path);
					m_active_scene->m_main_framebuffer = m_gui->get_viewport_framebuffer();
					m_selected_entity = nullptr;
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
						m_active_scene = std::make_shared<Scene>();
						m_active_scene->m_main_framebuffer = m_gui->get_viewport_framebuffer();
						m_selected_entity = nullptr;
					}
					//if (ImGui::MenuItem("open...")) {
					//	Filepath file = open_file_dialog("yaml (*.yaml)\0*.yaml\0all files\0*.*\0");
					//	if (!file.empty()) {
					//		m_active_scene = Scene::deserialize(file);
					//		m_active_scene->m_main_framebuffer = m_gui->get_viewport_framebuffer();
					//		m_selected_entity = nullptr;
					//	}
					//}
					//if (ImGui::MenuItem("save as...")) {
					//	Filepath file = save_file_dialog("yaml (*.yaml)\0*.yaml\0all files\0*.*\0");
					//	if (!file.empty()) {
					//		if (file.extension() != ".yaml") {
					//			file += ".yaml";
					//		}
					//		Scene::serialize(file, m_active_scene);
					//	}
					//}
					if (ImGui::MenuItem("exit")) {
						terminate();
					}
					ImGui::EndMenu();
				}
			};

		m_gui->m_draw_viewport_func =
			[&]() {
				if (m_selected_entity) {
					auto const& cam = m_active_scene->get_main_camera();
					if (cam) {
						ImVec2 image_min = ImGui::GetItemRectMin(); // top-left corner
						ImVec2 image_max = ImGui::GetItemRectMax(); // bottom-right corner
						ImVec2 present_size = ImVec2(image_max.x - image_min.x, image_max.y - image_min.y);

						auto transform = m_selected_entity->get_component<TransformComponent>().get_world_transform();

						ImGuizmo::SetRect(image_min.x, image_min.y, present_size.x, present_size.y);
						auto view = cam->get_component<CameraComponent>().get_view();
						auto proj = cam->get_component<CameraComponent>().get_proj();
						ImGuizmo::Manipulate(&view[0][0], &proj[0][0], m_current_gizmo_operation, m_current_gizmo_mode, &transform[0][0], NULL, NULL);

						m_is_using_gizmo = ImGuizmo::IsUsing() || ImGuizmo::IsOver();
						if (m_is_using_gizmo) {
							glm::vec3 translation{}, rotation{}, scale{};
							ImGuizmo::DecomposeMatrixToComponents(&transform[0][0], &translation.x, &rotation.x, &scale.x);
							m_selected_entity->get_component<TransformComponent>().m_location = translation;
							m_selected_entity->get_component<TransformComponent>().m_rotation = rotation;
							m_selected_entity->get_component<TransformComponent>().m_scale = scale;
						}
					}
				}
			};

		{
			//Pipeline::Description desc{};
			//desc.depth_test = true;
			//desc.blend = true;
			//desc.cull_mode = CullMode::Back;
			//desc.shader = g_runtime_context.m_asset_manager->get<Shader>("sprite_2d");
			//auto pipeline = Pipeline::build(desc);
			//auto material = std::make_shared<Material>(pipeline);
			/*m_material_instance = std::make_shared<MaterialInstance>("MI_Sprite2D", m_material);
			m_material_instance->m_override_variables["u_texture"].default_value.resource_id = tex0->get_resource_id();
			m_material_instance->m_override_variables["u_texture"].default_value.valid = true;*/
		}

		// picking system doesn��t need to be that precise
		// so we use default resolution here (512x512)
		m_picking = std::make_shared<PickingSystem>();
	}

	~EditorLayer() {

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
		return true;
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
		return true;
	}

	void on_imgui_render() override {
		m_gui->draw();

		if (ImGui::Begin("resource info")) {
			ImGui::Text("resources");
			if (ImGui::BeginTable("resources", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
				ImGui::TableSetupColumn("id");
				ImGui::TableSetupColumn("type");
				ImGui::TableSetupColumn("binding");
				ImGui::TableSetupColumn("ref count");
				ImGui::TableSetupColumn("info");
				ImGui::TableHeadersRow();

				ImGui::TableNextRow();
				for (auto const& resource : g_runtime_context.m_resource_manager->m_resources) {
					if (!resource) continue;
					ImGui::TableNextColumn(); ImGui::Text(std::to_string(resource->get_resource_id()).c_str());
					ImGui::TableNextColumn(); ImGui::Text(get_resource_name(resource->get_resource_type()).c_str());
					ImGui::TableNextColumn(); ImGui::Text(resource->get_binding() == INVALID_BINDING ? "none" : std::to_string(resource->get_binding()).c_str());
					ImGui::TableNextColumn(); ImGui::Text(std::to_string(resource->get_ref_count()).c_str());
					switch (resource->get_resource_type()) {
					case ResourceType::Image:
						ImGui::TableNextColumn(); ImGui::Text(get_image_info(g_runtime_context.m_resource_manager->get<Image>(resource->get_resource_id())).c_str()); break;
					case ResourceType::UniformBuffer:
						ImGui::TableNextColumn(); ImGui::Text(get_uniform_buffer_info(g_runtime_context.m_resource_manager->get<UniformBuffer>(resource->get_resource_id())).c_str()); break;
					}
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();

		show_scene_graph();
		show_properties();
		m_browser->draw();
		//show_shader_info(m_material->m_pipeline->m_shader);
		//show_material_info(m_material_instance);

		if (ImGui::Begin("debug")) {
			if (ImGui::RadioButton("v sync", g_runtime_context.m_window->is_v_sync_enabled())) {
				g_runtime_context.m_window->set_v_sync(!g_runtime_context.m_window->is_v_sync_enabled());
			}

			ImGui::Text(std::string("viewport_pixel_scale_x: " + std::to_string(m_gui->m_viewport_pixel_scale_x)).c_str());

			if (ImGui::Button("save imgui ini")) {
				fs::path ini_path = "imgui.ini";
				fs::path default_path = "editor/default.ini";
				fs::copy_file(ini_path, default_path, fs::copy_options::overwrite_existing);
			}
		}
		ImGui::End();
	}

private:
	std::shared_ptr<EditorGUI> m_gui;
	std::shared_ptr<Scene> m_active_scene;
	std::unique_ptr<ContentBrowser> m_browser;
	bool m_picked_from_viewport = false;
	std::shared_ptr<Entity> m_selected_entity = nullptr;

	std::shared_ptr<PickingSystem> m_picking;

	bool m_is_using_gizmo = false;
	ImGuizmo::OPERATION m_current_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
	ImGuizmo::MODE m_current_gizmo_mode = ImGuizmo::MODE::LOCAL;

	int m_fps_counter = 0;
	float m_fps_timer = 0.0;

	void show_scene_graph() {
		if (ImGui::Begin("scene")) {
			ImGui::Text("entities in scene: %d", m_active_scene->get_entity_count());
			ImGui::Separator();
			for (auto const& ent : m_active_scene->m_registry.view<TransformComponent>()) {
				auto entity = m_active_scene->cast_to_entity(ent);
				auto const& tag = entity->get_component<TagComponent>();
				if (ImGui::Selectable((tag.m_tag + "##" + std::to_string(tag.m_id)).c_str(), entity == m_selected_entity)) {
					m_selected_entity = entity; // Update selection
				}
				if (m_picked_from_viewport && entity == m_selected_entity) {
					ImGui::SetScrollHereY();
					m_picked_from_viewport = false;
				}
			}

			if (ImGui::BeginPopupContextWindow()) {
				if (ImGui::MenuItem("create empty entity")) {
					m_active_scene->create_entity("new entity");
				}
				ImGui::EndPopup();
			}
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

	void show_properties() {
		static char name_buffer[256] = {};

		if (ImGui::Begin("properties")) {
			if (m_selected_entity) {
				auto& tag = m_selected_entity->get_component<TagComponent>();
				strcpy_s(name_buffer, tag.m_tag.c_str());
				if (ImGui::InputText("##tag", name_buffer, IM_ARRAYSIZE(name_buffer))) {
					tag.m_tag = std::string(name_buffer);
				}

				if (ImGui::CollapsingHeader("transform", ImGuiTreeNodeFlags_DefaultOpen)) {
					auto& transform = m_selected_entity->get_component<TransformComponent>();
					ImGui::Indent();
					ImGui::DragFloat3("location", &transform.m_location[0], 0.01f);
					ImGui::DragFloat3("rotation", &transform.m_rotation[0], 0.1f);
					ImGui::DragFloat3("scale", &transform.m_scale[0], 0.01f);
					ImGui::Unindent();
				}

				if (m_selected_entity->has_component<SpriteComponent>()) {
					if (ImGui::CollapsingHeader("sprite", ImGuiTreeNodeFlags_DefaultOpen)) {
						auto& sprite = m_selected_entity->get_component<SpriteComponent>();
						ImGui::ColorEdit4("color", &sprite.m_color[0]);
						ImGui::Text("texture");
						ImGui::Indent();
						if (sprite.m_texture) {
							auto w = sprite.m_texture->m_image->get_description().m_width;
							auto h = sprite.m_texture->m_image->get_description().m_height;
							ImGui::Image(sprite.m_texture->m_image->get_native_handle(), ImVec2(64.0f * w / h, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
							ImGui::Text("guid: %s", sprite.m_texture->m_meta.guid.value.c_str());
							ImGui::Text("width: %d", w);
							ImGui::Text("height: %d", h);
							ImGui::Text("depth: %d", sprite.m_texture->m_image->get_description().m_depth);
							ImGui::Text("format: %s", get_image_format_name(sprite.m_texture->m_image->get_description().m_format).c_str());
							ImGui::Text("sampler Mode: %s", get_sampler_mode_name(sprite.m_texture->m_image->get_description().m_sampler_mode).c_str());
							ImGui::Text("wrap Mode: %s", get_wrap_mode_name(sprite.m_texture->m_image->get_description().m_wrap_mode).c_str());
							ImGui::RadioButton("mipmap", sprite.m_texture->m_image->get_description().m_mipmap);
						}
						else {
							ImGui::Text("-");
						}
						ImGui::Unindent();

						ImGui::DragFloat2("tiling scale", &sprite.m_tiling_scale[0], 0.01f);
						ImGui::DragFloat2("tiling offset", &sprite.m_tiling_offset[0], 0.01f);
						ImGui::Text("texcoords");
						ImGui::Indent();
						for (int i = 0; i < 4; ++i) {
							ImGui::DragFloat2(("texcoord " + std::to_string(i)).c_str(), &sprite.m_texcoords[i][0], 0.01f);
						}
						ImGui::Unindent();
					}

				}

				if (m_selected_entity->has_component<StaticMeshComponent>()) {
					if (ImGui::CollapsingHeader("static mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
						auto& mesh = m_selected_entity->get_component<StaticMeshComponent>();
						ImGui::Text("guid: %s", mesh.m_mesh->m_meta.guid.value.c_str());
						ImGui::Text("bound min: (%f, %f, %f)", mesh.m_mesh->m_bound_min.x, mesh.m_mesh->m_bound_min.y, mesh.m_mesh->m_bound_min.z);
						ImGui::Text("bound max: (%f, %f, %f)", mesh.m_mesh->m_bound_max.x, mesh.m_mesh->m_bound_max.y, mesh.m_mesh->m_bound_max.z);
						ImGui::Text("primitives");
						for (auto const& prim : mesh.m_mesh->m_primitives) {
							ImGui::Indent();
							ImGui::Text("triangle count: %d", prim.get_triangle_count());
							ImGui::Text("bound min: (%f, %f, %f)", prim.m_bound_min.x, prim.m_bound_min.y, prim.m_bound_min.z);
							ImGui::Text("bound max: (%f, %f, %f)", prim.m_bound_max.x, prim.m_bound_max.y, prim.m_bound_max.z);
							ImGui::Unindent();
						}
					}
				}

				if (m_selected_entity->has_component<CameraComponent>()) {
					if (ImGui::CollapsingHeader("camera", ImGuiTreeNodeFlags_DefaultOpen)) {
						auto& camera = m_selected_entity->get_component<CameraComponent>();
						ImGui::Checkbox("is perspective", &camera.m_is_perspective);
						if (camera.m_is_perspective) {
							ImGui::InputFloat("field of view", &camera.m_intrinsic.fov, 0.01f);
						}
						else {
							ImGui::InputFloat("frustum size", &camera.m_intrinsic.size, 0.01f);
						}
						ImGui::InputFloat("near", &camera.m_near, 0.01f);
						ImGui::InputFloat("far", &camera.m_far, 0.01f);
						ImGui::InputFloat("aspect ratio", &camera.m_aspect, 0.01f);
						ImGui::Checkbox("use fixed aspect", &camera.m_use_fixed_aspect);
						if (ImGui::RadioButton("is primary", camera.m_is_primary)) {
							if (!camera.m_is_primary) {
								m_active_scene->set_main_camera(m_selected_entity);
							}
						}
					}
				}

				if (m_selected_entity->has_component<ScriptComponent>()) {
					if (ImGui::CollapsingHeader("script", ImGuiTreeNodeFlags_DefaultOpen)) {
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
					component_context_menu<SpriteComponent>("sprite component", m_selected_entity);
					component_context_menu<ScriptComponent>("script component", m_selected_entity, m_selected_entity);
					ImGui::EndPopup();
				}
			}
		}
		ImGui::End();
	}

	void show_shader_info(std::shared_ptr<Shader> const& shader) {
		if (ImGui::Begin("shader info")) {
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
		ImGui::End();
	}

	void show_material_info(std::shared_ptr<MaterialInstance> const& material) {
		if (ImGui::Begin("material instance")) {
			ImGui::Text("name: %s", material->m_meta.path.generic_string());
			ImGui::Text("parent material: %s", material->m_material->m_meta.path.generic_string());
			ImGui::Text("shader name: %s", material->m_material->m_pipeline->m_shader->get_name().c_str());

			ImGui::Text("variables");
			for (auto& [name, var] : material->m_override_variables) {
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

				if (var.type == DataType::Sampler2D && var.default_value.resource_id != INVALID_INDEX) {
					auto image = g_runtime_context.m_resource_manager->get<Image2D>(var.default_value.resource_id);
					auto w = image->get_description().m_width;
					auto h = image->get_description().m_height;
					ImGui::Image(image->get_native_handle(), ImVec2(64.0f * w / h, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
				}

				if (!var.default_value.valid) {
					ImGui::EndDisabled();
				}
			}
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
