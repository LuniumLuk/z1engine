#include "iostream"
#include "z1engine.h"
#include "glad/glad.h"
#include "glm/gtc/matrix_transform.hpp"
#include "gui.h"
#include "input_mgr.h"
#include "camera_ctrl.h"
#include "picking_system.h"

using namespace z1;
namespace fs = std::filesystem;

struct CameraCtrlScript : ScriptBase {
	void on_attach() override {
		std::cout << "CameraCtrlScript::on_attach, entity tag: " << get_component<TagComponent>().m_tag << std::endl;
	}

	void on_update(float delta_time) override {
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_A)) {
			get_component<TransformComponent>().m_location.x -= 0.1f;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_D)) {
			get_component<TransformComponent>().m_location.x += 0.1f;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_W)) {
			get_component<TransformComponent>().m_location.y += 0.1f;
		}
		if (g_runtime_context.m_input_system->is_key_pressed(KEY_S)) {
			get_component<TransformComponent>().m_location.y -= 0.1f;
		}

		if (g_runtime_context.m_input_system->is_key_pressed(KEY_F)) {
			destroy(); // destroy this script itself
		}
	}

	void on_detach() override {
		std::cout << "CameraCtrlScript::on_detach" << std::endl;
	}
};


struct EditorLayer : Layer {
	EditorLayer() {
		m_gui = std::make_shared<EditorGUI>();
		m_active_scene = std::make_shared<Scene>();

		auto persp_cam = m_active_scene->create_entity("Persp Camera");
		persp_cam->add_component<CameraComponent>();

		auto ortho_cam = m_active_scene->create_entity("Ortho Camera");
		auto& ortho_cc = ortho_cam->add_component<CameraComponent>();
		ortho_cc.m_is_perspective = false;
		ortho_cc.m_near = -20.0f;
		ortho_cc.m_far = 20.0f;
		ortho_cc.m_intrinsic.size = 4.0f; // frustum size
		m_active_scene->set_main_camera(ortho_cam);

		// test script
		ortho_cam->attach_script<CameraCtrlScript>();
		persp_cam->attach_script<CameraCtrlScript>();

		m_active_scene->m_main_framebuffer = m_gui->get_viewport_framebuffer();

		m_ortho_camera_ctrl = std::make_shared<Generic2DCameraController>(ortho_cam);
		m_persp_camera_ctrl = std::make_shared<HoveringCameraController>(persp_cam);

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
					if (m_ortho_camera_ctrl->get_camera()->get_component<CameraComponent>().m_is_primary) {
						ImGui::Text("camera ctrl:");
						ImGui::InputFloat("move speed", &m_ortho_camera_ctrl->m_move_speed, 0.1f, 1.0f);
						ImGui::InputFloat("zoom speed", &m_ortho_camera_ctrl->m_zoom_speed, 0.1f, 1.0f);
					}
					else if (m_persp_camera_ctrl->get_camera()->get_component<CameraComponent>().m_is_primary) {
						ImGui::Text("camera ctrl:");
						ImGui::InputFloat("rotate speed", &m_persp_camera_ctrl->m_rotate_speed, 0.1f, 1.0f);
						ImGui::InputFloat("move speed", &m_persp_camera_ctrl->m_move_speed, 0.1f, 1.0f);
					}
				}
				ImGui::End();

				ImGui::PopStyleColor();
			};

		m_texture = g_runtime_context.m_asset_manager->get<Image2D>("texture/awesomeface.png");
		m_checker = g_runtime_context.m_asset_manager->get<Image2D>("texture/tira-checker.jpg");
		// TODO: need support for image settings (sampler mode and wrap mode)
		m_atlas = g_runtime_context.m_asset_manager->get<Image2D>("texture/roguelikeSheet_transparent.png");

		{
			Pipeline::Description desc{};
			desc.depth_test = true;
			desc.blend = true;
			desc.cull_mode = CullMode::Back;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/sprite_2d.glsl");
			auto pipeline = Pipeline::build(desc);
			m_material = std::make_shared<Material>("M_Sprite2D", pipeline);
			m_material_instance = std::make_shared<MaterialInstance>("MI_Sprite2D", m_material);
			m_material_instance->m_override_variables["u_texture"].default_value.resource_id = m_texture->get_resource_id();
			m_material_instance->m_override_variables["u_texture"].default_value.valid = true;
		}

		{
			auto ent = m_active_scene->create_entity("Square_0");
			ent->add_component<SpriteComponent>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), m_texture);
		}

		{
			auto ent = m_active_scene->create_entity("Square_1");
			ent->add_component<SpriteComponent>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), m_checker);
			ent->get_component<TransformComponent>().m_location = glm::vec3(0.0f, 0.0f, -9.5f);
			ent->get_component<TransformComponent>().m_scale = glm::vec3(10.0f, 10.0f, 1.0f);
		}

		{
			auto ent = m_active_scene->create_entity("Mesh_0");
			ent->add_component<StaticMeshComponent>(
				g_runtime_context.m_asset_manager->get<StaticMesh>("mesh/bunny.obj")
			);
			ent->get_component<TransformComponent>().m_location = glm::vec3(0.0f, 0.0f, -5.0f);
		}

		{
			auto ent = m_active_scene->create_entity("Mesh_1");
			ent->add_component<StaticMeshComponent>(
				g_runtime_context.m_asset_manager->get<StaticMesh>("fireplace_room/fireplace_room.obj")
			);
		}

		io::load_gltf_scene(m_active_scene, "editor/asset/mesh/DamagedHelmet.glb");

		uint32_t quad_rows = 4;
		uint32_t quad_cols = 4;
		float quad_stride = 0.15f;
		float quad_size = 0.1f;

		uint32_t tile_size = 16;
		uint32_t stride = 1;
		for (uint32_t i = 0; i < quad_rows; ++i) {
			for (uint32_t j = 0; j < quad_cols; ++j) {
				auto tile = SubImage2D::create(m_atlas, i * (tile_size + stride), j * (tile_size + stride), tile_size, tile_size, true);
				auto ent = m_active_scene->create_entity("SubImage2D_" + std::to_string(i * quad_rows + j));
				ent->add_component<SpriteComponent>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), tile);
				ent->get_component<TransformComponent>().m_location = glm::vec3(-(float)i * quad_stride - 0.2f, -(float)j * quad_stride - 0.2f, 1.0f);
				ent->get_component<TransformComponent>().m_scale = glm::vec3(quad_size, quad_size, 1.0f);
			}
		}

		for (uint32_t i = 0; i < quad_rows; ++i) {
			for (uint32_t j = 0; j < quad_cols; ++j) {
				auto ent = m_active_scene->create_entity("Quad_" + std::to_string(i * quad_rows + j));
				ent->add_component<SpriteComponent>(glm::vec4((float)(i % quad_rows) / quad_rows, (float)(j % quad_cols) / quad_cols, 1.0f, 1.0f));
				ent->get_component<TransformComponent>().m_location = glm::vec3(i * quad_stride, j * quad_stride, 0.1f);
				ent->get_component<TransformComponent>().m_scale = glm::vec3(quad_size, quad_size, 1.0f);
			}
		}

		m_picking = std::make_shared<PickingSystem>(
			m_gui->get_viewport_framebuffer()->get_description().width,
			m_gui->get_viewport_framebuffer()->get_description().height);
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
		m_ortho_camera_ctrl->m_drag_speed = m_gui->m_viewport_pixel_scale_x;

		m_input_mgr.update(delta_time);
		if (m_gui->is_viewport_focused()) {
			if (m_ortho_camera_ctrl->get_camera()->get_component<CameraComponent>().m_is_primary) {
				//m_ortho_camera_ctrl->update(m_input_mgr);
			}
			else if (m_persp_camera_ctrl->get_camera()->get_component<CameraComponent>().m_is_primary) {
				//m_persp_camera_ctrl->update(m_input_mgr);
			}
		}
		m_input_mgr.reset();

		m_active_scene->on_update(delta_time);
	}

	void on_event(Event& event) override {
		auto dispatcher = EventDispatcher(event);
		dispatcher.dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(EditorLayer::on_mouse_pressed));
		m_input_mgr.on_event(event);
	}

	bool on_mouse_pressed(MouseButtonPressedEvent& event) {
		if (event.GetButton() == MOUSE_BUTTON_LEFT) {
			if (m_gui->is_viewport_focused() && m_gui->is_viewport_hovered()) {
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
		show_shader_info(m_material->m_pipeline->m_shader);
		show_material_info(m_material_instance);

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
	bool m_picked_from_viewport = false;
	std::shared_ptr<Entity> m_selected_entity = nullptr;
	std::shared_ptr<Generic2DCameraController> m_ortho_camera_ctrl;
	std::shared_ptr<HoveringCameraController> m_persp_camera_ctrl;
	InputManager m_input_mgr;
	std::shared_ptr<Image2D> m_texture;
	std::shared_ptr<Image2D> m_checker;
	std::shared_ptr<Image2D> m_atlas;

	std::shared_ptr<Material> m_material;
	std::shared_ptr<MaterialInstance> m_material_instance;

	std::shared_ptr<PickingSystem> m_picking;

	int m_fps_counter = 0;
	float m_fps_timer = 0.0;

	void show_scene_graph() {
		if (ImGui::Begin("scene")) {
			ImGui::Text("entities in scene: %d", m_active_scene->get_entity_count());
			ImGui::Separator();
			for (auto const& ent : m_active_scene->m_registry.view<TransformComponent>()) {
				auto entity = m_active_scene->cast_to_entity(ent);
				if (ImGui::Selectable(entity->get_component<TagComponent>().m_tag.c_str(), entity == m_selected_entity)) {
					m_selected_entity = entity; // Update selection
				}
				if (m_picked_from_viewport && entity == m_selected_entity) {
					ImGui::SetScrollHereY();
					m_picked_from_viewport = false;
				}
			}
		}
		ImGui::End();
	}

	void show_properties() {
		if (ImGui::Begin("properties")) {
			if (m_selected_entity) {
				auto& tag = m_selected_entity->get_component<TagComponent>();
				ImGui::Text(tag.m_tag.c_str());

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
							auto w = sprite.m_texture->get_description().m_width;
							auto h = sprite.m_texture->get_description().m_height;
							ImGui::Image(sprite.m_texture->get_native_handle(), ImVec2(64.0f * w / h, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
							ImGui::Text("guid: %s", sprite.m_texture->m_guid.value.c_str());
							ImGui::Text("width: %d", w);
							ImGui::Text("height: %d", h);
							ImGui::Text("depth: %d", sprite.m_texture->get_description().m_depth);
							ImGui::Text("format: %s", get_image_format_name(sprite.m_texture->get_description().m_format).c_str());
							ImGui::Text("sampler Mode: %s", get_sampler_mode_name(sprite.m_texture->get_description().m_sampler_mode).c_str());
							ImGui::Text("wrap Mode: %s", get_wrap_mode_name(sprite.m_texture->get_description().m_wrap_mode).c_str());
							ImGui::RadioButton("mipmap", sprite.m_texture->get_description().m_mipmap);
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
						ImGui::Text("guid: %s", mesh.m_mesh->m_guid.value.c_str());
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
			ImGui::Text("name: %s", material->m_name.c_str());
			ImGui::Text("parent material: %s", material->m_material->m_name.c_str());
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
