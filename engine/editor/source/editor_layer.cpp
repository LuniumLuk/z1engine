#include "editor_layer.h"

void EditorSettings::save() {
	YAML::Emitter yaml;
	yaml << YAML::BeginMap;
	yaml << YAML::Key << "last_opened_scene_guid" << YAML::Value << last_opened_scene_guid;
	yaml << YAML::Key << "show_light_gizmos" << YAML::Value << show_light_gizmos;
	yaml << YAML::Key << "light_gizmo_size" << YAML::Value << light_gizmo_size;
	yaml << YAML::Key << "curr_resolution" << YAML::Value << curr_resolution;
	yaml << YAML::Key << "show_skeleton_guizmos" << YAML::Value << show_skeleton_guizmos;
	yaml << YAML::Key << "skeleton_gizmo_size" << YAML::Value << skeleton_gizmo_size;
	yaml << YAML::EndMap;

	std::ofstream fout("editor_settings.yaml");
	fout << yaml.c_str();
}

void EditorSettings::load() {
	if (!fs::exists("editor_settings.yaml"))
		return;

	try {
		YAML::Node yaml = YAML::LoadFile("editor_settings.yaml");
		if (yaml["last_opened_scene_guid"])
			last_opened_scene_guid = yaml["last_opened_scene_guid"].as<std::string>();
		if (yaml["show_light_gizmos"])
			show_light_gizmos = yaml["show_light_gizmos"].as<bool>();
		if (yaml["light_gizmo_size"])
			light_gizmo_size = yaml["light_gizmo_size"].as<float>();
		if (yaml["curr_resolution"])
			curr_resolution = yaml["curr_resolution"].as<uint32_t>();
		if (yaml["show_skeleton_guizmos"])
			show_skeleton_guizmos = yaml["show_skeleton_guizmos"].as<bool>();
		if (yaml["skeleton_gizmo_size"])
			skeleton_gizmo_size = yaml["skeleton_gizmo_size"].as<float>();
	} catch (...) {
		std::cout << "failed to load editor settings" << std::endl;
	}
}

EditorLayer::EditorLayer() {
	m_one_frame = g_args.get<int>("one-frame", -1);
	m_settings.load();
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
				ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - 10.0f,
				ImGui::GetWindowPos().y + 30.0f
			);
			ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f); // right-top pivot

			ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
			ImGui::SetNextWindowBgAlpha(0.7f);

			if (ImGui::Begin("overlay on viewport", nullptr,
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoDecoration |
				ImGuiWindowFlags_AlwaysAutoResize |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoFocusOnAppearing |
				ImGuiWindowFlags_NoNav))
			{
				if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
					auto const& stats = g_runtime_context.m_graphics_context->m_stats;
					ImGui::Text("Visible Objects: %u", stats.visible_objects);
					ImGui::Text("Culled Objects: %u", stats.culled_objects);
					ImGui::Text("FPS: %.1f", stats.fps);
					ImGui::Text("5%% Low: %.1f", stats.low_5_percent);
					ImGui::Text("1%% Low: %.1f", stats.low_1_percent);
					ImGui::Text("Frame Time: %.2f ms", stats.frame_time);
					ImGui::Text("Draw Calls: %u", stats.draw_calls);
					for (auto const& counter : stats.counters) {
						ImGui::Text("-- %s: %u", counter.first.c_str(), counter.second);
					}
				}
			}
			ImGui::End();
		};

	m_gui->m_draw_menu_bar_items_func =
		[&]() {
			if (ImGui::BeginMenu("file")) {
				if (ImGui::MenuItem("new")) {
					load_scene();
				}
				if (ImGui::MenuItem("save")) {
					g_runtime_context.m_scene->save();
				}
				if (ImGui::MenuItem("exit")) {
					terminate();
				}
				ImGui::EndMenu();
			}
		};

	m_gui->m_draw_viewport_func =
		[&]() {
			auto const& cam = g_runtime_context.m_scene->get_main_camera();
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
				auto light_view = g_runtime_context.m_scene->m_registry.view<TransformComponent const, LightComponent const>();
				for (auto [entity, transform, light] : light_view.each()) {
					// Don't draw cube for selected entity to avoid clutter with the manipulator
					if (m_selected_entity && m_selected_entity->get_component<TagComponent>().m_id == g_runtime_context.m_scene->m_registry.get<TagComponent>(entity).m_id) {
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

			// Visualize PostProcess Volumes
			{
				auto pp_view = g_runtime_context.m_scene->m_registry.view<TransformComponent const, PostprocessVolumeComponent const>();
				for (auto [entity, transform, pp] : pp_view.each()) {
					if (!pp.enabled) continue;

					glm::mat4 model = transform.get_world_transform();
					glm::vec3 corners[8] = {
						{ -0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, -0.5f }, { -0.5f, 0.5f, -0.5f },
						{ -0.5f, -0.5f, 0.5f }, { 0.5f, -0.5f, 0.5f }, { 0.5f, 0.5f, 0.5f }, { -0.5f, 0.5f, 0.5f }
					};

					glm::vec3 world_corners[8];
					for (int i = 0; i < 8; ++i) {
						world_corners[i] = glm::vec3(model * glm::vec4(corners[i], 1.0f));
					}

					int edges[12][2] = {
						{0,1}, {1,2}, {2,3}, {3,0},
						{4,5}, {5,6}, {6,7}, {7,4},
						{0,4}, {1,5}, {2,6}, {3,7}
					};

					auto draw_list = ImGui::GetWindowDrawList();
					glm::mat4 view_proj = proj * view;

					for (int i = 0; i < 12; ++i) {
						glm::vec3 p1 = world_corners[edges[i][0]];
						glm::vec3 p2 = world_corners[edges[i][1]];

						glm::vec4 cp1 = view_proj * glm::vec4(p1, 1.0f);
						glm::vec4 cp2 = view_proj * glm::vec4(p2, 1.0f);

						if (cp1.w > 0.1f && cp2.w > 0.1f) {
							glm::vec3 ndc1 = glm::vec3(cp1) / cp1.w;
							glm::vec3 ndc2 = glm::vec3(cp2) / cp2.w;

							ImVec2 sp1 = {
								(ndc1.x + 1.0f) * 0.5f * present_size.x + image_min.x,
								(1.0f - ndc1.y) * 0.5f * present_size.y + image_min.y
							};
							ImVec2 sp2 = {
								(ndc2.x + 1.0f) * 0.5f * present_size.x + image_min.x,
								(1.0f - ndc2.y) * 0.5f * present_size.y + image_min.y
							};

							draw_list->AddLine(sp1, sp2, IM_COL32(0, 255, 255, 255), 2.0f);
						}
					}
				}
			}

			// Visualize skeletons using ImGuizmo::DrawLines
			if (m_settings.show_skeleton_guizmos) {
				auto skel_view = g_runtime_context.m_scene->m_registry.view<TransformComponent const, AnimationComponent const, SkeletalMeshComponent const>();
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

EditorLayer::~EditorLayer() {
	m_settings.curr_resolution = m_gui->m_current_resolution;
	m_settings.save();
}

void EditorLayer::on_attach() {

}

void EditorLayer::on_update(float delta_time) {
	m_fps_timer += delta_time;
	m_fps_counter += 1;
	if (m_fps_timer >= 1.0) {
		std::string title = "z1 engine";
		title += " (" + g_runtime_context.m_scene->m_meta.name() + ")";
		title += " fps: " + std::to_string(m_fps_counter);

		g_runtime_context.m_window->set_window_title(title);
		m_fps_timer = 0.0;
		m_fps_counter = 0;
	}
	g_runtime_context.m_scene->on_update(delta_time);
	if (g_runtime_context.m_global->render_mode == RenderMode::Deferred) {
		g_runtime_context.m_renderer_deferred->draw(g_runtime_context.m_scene, m_gui->get_viewport_framebuffer());
	} else {
		g_runtime_context.m_renderer_forward->draw(g_runtime_context.m_scene, m_gui->get_viewport_framebuffer());
	}
	//g_runtime_context.m_renderer_2d->draw(g_runtime_context.m_scene, m_gui->get_viewport_framebuffer());
	m_picking->render(g_runtime_context.m_scene);

	g_runtime_context.m_graphics_context->bind_framebuffer(g_runtime_context.m_graphics_context->m_swapchain_framebuffer);

	if (m_one_frame >= 0 && m_one_frame == m_frame_count) {
		save_screenshot();
		terminate();
	}
	m_frame_count += 1;
}

void EditorLayer::on_event(Event& event) {
	auto dispatcher = EventDispatcher(event);
	dispatcher.dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(EditorLayer::on_mouse_pressed));
	dispatcher.dispatch<KeyPressedEvent>(BIND_EVENT_FN(EditorLayer::on_key_pressed));
}

bool EditorLayer::on_key_pressed(KeyPressedEvent& event) {
	switch (event.get_keycode()) {
	case KEY_W:
		m_current_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE; break;
	case KEY_E:
		m_current_gizmo_operation = ImGuizmo::OPERATION::ROTATE; break;
	case KEY_R:
		m_current_gizmo_operation = ImGuizmo::OPERATION::SCALE; break;
	case KEY_G:
		m_selected_entity = nullptr; break;
	default:
		break;
	}
	return false;
}

bool EditorLayer::on_mouse_pressed(MouseButtonPressedEvent& event) {
	if (event.get_button() == MOUSE_BUTTON_LEFT) {
		if (/*m_gui->is_viewport_focused() && */m_gui->is_viewport_hovered() && !m_is_using_gizmo) {
			auto start = std::chrono::high_resolution_clock::now();

			m_picking->render(g_runtime_context.m_scene);

			float x = 0.0f;
			float y = 0.0f;
			m_gui->get_mouse_cursor_on_viewport(&x, &y);

			auto object_id = m_picking->query(x, y);
			if (object_id == INVALID_INDEX) {
				m_selected_entity = nullptr;
			}
			else {
				m_selected_entity = g_runtime_context.m_scene->m_entities[object_id];
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

void EditorLayer::load_scene(std::shared_ptr<Scene> const& scene /*= nullptr*/) {
	if (scene) {
		g_runtime_context.m_scene = scene;
		m_settings.last_opened_scene_guid = g_runtime_context.m_scene->m_meta.guid.value;
	}
	else {
		auto folder = m_browser->get_curr_dir();
		auto path = g_runtime_context.m_asset_manager->next_path_available(folder / "new_scene");
		g_runtime_context.m_scene = Scene::create(path);
		if (!g_runtime_context.m_scene) {
			return;
		}
		g_runtime_context.m_scene->save();
		m_settings.last_opened_scene_guid = g_runtime_context.m_scene->m_meta.guid.value;
	}

	m_selected_entity = nullptr;

	// setup editor viewport camera
	auto camera = g_runtime_context.m_scene->create_transient_entity("[Editor] Viewport Camera");
	auto& cam_comp = camera->add_component<CameraComponent>();
	auto& trans_comp = camera->get_component<TransformComponent>();

	if (g_runtime_context.m_scene->m_editor_camera_data.is_valid) {
		cam_comp.m_aspect = g_runtime_context.m_scene->m_editor_camera_data.camera.m_aspect;
		cam_comp.m_near = g_runtime_context.m_scene->m_editor_camera_data.camera.m_near;
		cam_comp.m_far = g_runtime_context.m_scene->m_editor_camera_data.camera.m_far;
		cam_comp.m_use_fixed_aspect = g_runtime_context.m_scene->m_editor_camera_data.camera.m_use_fixed_aspect;
		cam_comp.m_intrinsic = g_runtime_context.m_scene->m_editor_camera_data.camera.m_intrinsic;

		trans_comp.m_location = g_runtime_context.m_scene->m_editor_camera_data.transform.m_location;
		trans_comp.m_rotation = g_runtime_context.m_scene->m_editor_camera_data.transform.m_rotation;
		trans_comp.m_scale = g_runtime_context.m_scene->m_editor_camera_data.transform.m_scale;
	}
	else {
		trans_comp.m_location = { 0.0f, 0.0f, 5.0f };
	}

	camera->attach_script<HoveringCameraCtrlScript>(m_gui);
	g_runtime_context.m_scene->set_main_camera(camera);
}

void EditorLayer::save_screenshot() {
	auto& fb = m_gui->get_viewport_framebuffer();
	uint32_t width = fb->get_width();
	uint32_t height = fb->get_height();

	// Allocate buffer for RGBA8 pixels
	std::vector<unsigned char> pixels(width * height * 4);

	fb->read_pixels(0, 0, 0, width, height, pixels.data());

	// *** OPTIONAL: flip vertically for correct orientation ***
	std::vector<unsigned char> flipped(width * height * 4);
	for (uint32_t y = 0; y < height; ++y) {
		std::memcpy(&flipped[y * width * 4], &pixels[(height - 1 - y) * width * 4], width * 4);
	}

	std::string filename = "screenshot.png";

	// Save using stb_image_write (PNG)
	if (!stbi_write_png(filename.c_str(), width, height, 4, flipped.data(), width * 4)) {
		std::cerr << "Failed to write image!" << std::endl;
	} else {
		std::cout << "Saved: " << filename << std::endl;
	}
}

void EditorLayer::on_imgui_render() {
	m_gui->draw();

	if (ImGui::Begin("debug")) {
		if (ImGui::RadioButton("v sync", g_runtime_context.m_window->is_v_sync_enabled())) {
			g_runtime_context.m_window->set_v_sync(!g_runtime_context.m_window->is_v_sync_enabled());
		}

		ImGui::Text(std::string("viewport_pixel_scale_x: " + std::to_string(m_gui->m_viewport_pixel_scale_x)).c_str());

		if (ImGui::Button("save default.ini")) {
			ImGui::SaveIniSettingsToDisk("engine/config/default.ini");
		}

		if (ImGui::Button("save screenshot")) {
			save_screenshot();
		}

		ImGui::DragFloat("light gizmo size", &m_settings.light_gizmo_size, 0.01f, 0.0f, 1.0f);
		ImGui::Checkbox("show light gizmos", &m_settings.show_light_gizmos);
		ImGui::DragFloat("skeleton gizmo size", &m_settings.skeleton_gizmo_size, 0.01f, 0.0f, 1.0f);
		ImGui::Checkbox("show skeleton gizmos", &m_settings.show_skeleton_guizmos);

		if (ImGui::Button("use editor camera")) {
			use_editor_camera();
		}
	}
	ImGui::End();

	show_asset_info();
	show_scene_graph();
	show_properties(m_selected_entity);
	show_settings();
	m_browser->draw();
}

void EditorLayer::use_editor_camera() {
	for (auto& ent : g_runtime_context.m_scene->m_transient_entities) {
		if (ent && ent->has_component<CameraComponent>() && ent->get_component<TagComponent>().m_tag.find("[Editor] Viewport Camera") != std::string::npos) {
			g_runtime_context.m_scene->set_main_camera(ent);
			return;
		}
	}
}

void EditorLayer::show_scene_graph() {
	if (ImGui::Begin("scene")) {
		ImGui::Text("entities in scene: %llu", g_runtime_context.m_scene->get_entity_count());
		ImGui::Separator();

		std::unordered_map<TransformComponent*, Entity*> transform_to_entity;
		std::unordered_map<Entity*, std::vector<std::shared_ptr<Entity>>> children;
		std::vector<std::shared_ptr<Entity>> roots;

		// 1. Map transforms to entities
		for (auto const& ent : g_runtime_context.m_scene->m_entities) {
			if (ent) {
				transform_to_entity[&ent->get_component<TransformComponent>()] = ent.get();
			}
		}

		// 2. Build hierarchy
		for (auto const& ent : g_runtime_context.m_scene->m_entities) {
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
					for (auto& e : g_runtime_context.m_scene->m_entities) {
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
							auto new_entities = prefab->instantiate(g_runtime_context.m_scene);
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
					g_runtime_context.m_scene->destroy_entity(entity);
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
						prefab->instantiate(g_runtime_context.m_scene);
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		// Right click on empty space to create entity
		if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
			if (ImGui::MenuItem("create empty entity")) {
				g_runtime_context.m_scene->create_entity("new entity");
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

void EditorLayer::show_asset_info() {
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
			ImGui::Text("shader name: %s", mat->get_shader()->get_name().c_str());

			ImGui::Text("variables");
			for (auto& [name, var] : mat->m_variables) {
				if (!var.visible) continue;

				ImGui::Text(name.c_str());
				ImGui::Indent();
				ImGui::BeginDisabled();
				switch (var.type) {
				case DataType::Float: ImGui::InputFloat(("##" + name).c_str(), var.default_value.vec); break;
				case DataType::Float2: ImGui::InputFloat2(("##" + name).c_str(), var.default_value.vec); break;
				case DataType::Float3: ImGui::ColorEdit3(("##" + name).c_str(), var.default_value.vec); break;
				case DataType::Float4: ImGui::ColorEdit4(("##" + name).c_str(), var.default_value.vec); break;
				case DataType::Int: ImGui::InputInt(("##" + name).c_str(), var.default_value.ivec); break;
				case DataType::Int2: ImGui::InputInt2(("##" + name).c_str(), var.default_value.ivec); break;
				case DataType::Int3: ImGui::InputInt3(("##" + name).c_str(), var.default_value.ivec); break;
				case DataType::Int4: ImGui::InputInt4(("##" + name).c_str(), var.default_value.ivec); break;
				case DataType::Sampler2D: ImGui::Text(name.c_str()); break;
				case DataType::Sampler2DArray: ImGui::Text(name.c_str()); break;
				case DataType::SamplerCube: ImGui::Text(name.c_str()); break;
				}

				ImGui::EndDisabled();
				ImGui::Unindent();

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
			ImGui::Text("shader name: %s", mi->m_material->get_shader()->get_name().c_str());

			ImGui::Text("override variables");
			for (auto& [name, var] : mi->m_override_variables) {
				if (!var.visible) continue;

				ImGui::Checkbox(name.c_str(), &var.default_value.valid);
				ImGui::Indent();
				if (!var.default_value.valid) {
					ImGui::BeginDisabled();
				}
				switch (var.type) {
				case DataType::Float: ImGui::InputFloat(("##" + name).c_str(), var.default_value.vec); break;
				case DataType::Float2: ImGui::InputFloat2(("##" + name).c_str(), var.default_value.vec); break;
				case DataType::Float3: ImGui::ColorEdit3(("##" + name).c_str(), var.default_value.vec); break;
				case DataType::Float4: ImGui::ColorEdit4(("##" + name).c_str(), var.default_value.vec); break;
				case DataType::Int: ImGui::InputInt(("##" + name).c_str(), var.default_value.ivec); break;
				case DataType::Int2: ImGui::InputInt2(("##" + name).c_str(), var.default_value.ivec); break;
				case DataType::Int3: ImGui::InputInt3(("##" + name).c_str(), var.default_value.ivec); break;
				case DataType::Int4: ImGui::InputInt4(("##" + name).c_str(), var.default_value.ivec); break;
				}

				if (!var.default_value.valid) {
					ImGui::EndDisabled();
				}
				ImGui::Unindent();

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
				auto ent = g_runtime_context.m_scene->create_entity(m_selected_asset->name());
				ent->add_component<StaticMeshComponent>(
					g_runtime_context.m_asset_manager->get<StaticMesh>(m_selected_asset->guid)
				);
			}
		}
		else if (m_selected_asset->type == "skeletal mesh") {
			if (ImGui::Button("add to scene")) {
				auto ent = g_runtime_context.m_scene->create_entity(m_selected_asset->name());
				ent->add_component<SkeletalMeshComponent>(
					g_runtime_context.m_asset_manager->get<SkeletalMesh>(m_selected_asset->guid));
			}
		}
		else if (m_selected_asset->type == "prefab") {
			if (ImGui::Button("instantiate in scene")) {
				auto prefab = Prefab::load(m_selected_asset->guid);
				if (prefab) {
					prefab->instantiate(g_runtime_context.m_scene);
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

void EditorLayer::show_settings() {
	if (ImGui::Begin("settings")) {
		show_type_fields(g_runtime_context.m_global.get(), TYPE_NAME(GlobalSettings), true);
	}
	ImGui::End();
}

std::string EditorLayer::get_image_info(Image* image) {
	auto const& desc = image->get_description();
	std::string info = "";

	info += std::to_string(desc.m_width) + "x" + std::to_string(desc.m_height) + " ";
	info += get_image_format_name(desc.m_format) + " ";
	info += get_sampler_mode_name(desc.m_sampler_mode) + " ";
	info += get_wrap_mode_name(desc.m_wrap_mode);

	return info;
}

std::string EditorLayer::get_uniform_buffer_info(UniformBuffer* buffer) {
	std::string info = "size: ";

	info += std::to_string(buffer->get_size()) + " byte";

	return info;
}
