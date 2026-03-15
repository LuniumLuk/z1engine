#include "type_field.h"

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

#define SHOW_FLOAT_FIELD(num, value)                                       \
	float min = field.get_widget_value<float>("min", -1e6f);               \
	float max = field.get_widget_value<float>("max",  1e6f);               \
	float step = field.get_widget_value<float>("step", 0.01f);             \
	if (field.is_widget_type("slider")) {                                  \
		ImGui::SliderFloat##num(widget_name.c_str(), value, min, max);     \
	}                                                                      \
	else if (field.is_widget_type("drag")) {                               \
		ImGui::DragFloat##num(widget_name.c_str(), value, step, min, max); \
	}                                                                      \
	else {                                                                 \
		ImGui::InputFloat##num(widget_name.c_str(), value);                \
	}

#define SHOW_FLOAT_FIELD_WITH_COLOR(num, value)                            \
	float min = field.get_widget_value<float>("min", -1e6f);               \
	float max = field.get_widget_value<float>("max",  1e6f);               \
	float step = field.get_widget_value<float>("step", 0.01f);             \
	if (field.is_widget_type("color")) {                                   \
		ImGui::ColorEdit##num(widget_name.c_str(), value);                 \
	}                                                                      \
	else if (field.is_widget_type("slider")) {                             \
		ImGui::SliderFloat##num(widget_name.c_str(), value, min, max);     \
	}                                                                      \
	else if (field.is_widget_type("drag")) {                               \
		ImGui::DragFloat##num(widget_name.c_str(), value, step, min, max); \
	}                                                                      \
	else {                                                                 \
		ImGui::InputFloat##num(widget_name.c_str(), value);                \
	}

#define ACCEPT_PAYLOAD(asset_type, meta_type)                              \
	accept_payload("ASSET_ITEM", [&](void* data) {                         \
		AssetMeta* meta = *(AssetMeta**)data;                              \
		if (meta->type == meta_type) {                                     \
			value = asset_type::load(meta->guid);                          \
		}                                                                  \
	});

void show_value(void* ptr, std::type_info const& type, std::string const& name, FieldInfo const& field, const EnumInfo* enum_info /*= nullptr*/) {
	std::string widget_name = "##" + name;
	ImGui::SetNextItemWidth(-1.0f);

	if (enum_info) {
		int& value = *reinterpret_cast<int*>(ptr);
		std::string current_item_name = "Unknown";
		for (auto const& item : enum_info->items) {
			if (item.value == value) {
				current_item_name = item.name;
				break;
			}
		}

		if (ImGui::BeginCombo(widget_name.c_str(), current_item_name.c_str())) {
			for (auto const& item : enum_info->items) {
				bool is_selected = (value == item.value);
				if (ImGui::Selectable(item.name.c_str(), is_selected)) {
					value = item.value;
				}
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		return;
	}

	if (type == typeid(bool)) {
		bool& value = *reinterpret_cast<bool*>(ptr);
		if (field.is_widget_type("radio")) {
			if (ImGui::RadioButton(widget_name.c_str(), value)) {
				value = !value;
			}
		}
		else {
			ImGui::Checkbox(widget_name.c_str(), &value);
		}
	}
	else if (type == typeid(float)) {
		float& value = *reinterpret_cast<float*>(ptr);
		SHOW_FLOAT_FIELD(, &value)
	}
	else if (type == typeid(int)) {
		int& value = *reinterpret_cast<int*>(ptr);
		int min = field.get_widget_value<int>("min", -10000);
		int max = field.get_widget_value<int>("max", 10000);
		int step = field.get_widget_value<int>("step", 1);
		if (field.is_widget_type("slider")) {
			ImGui::SliderInt(widget_name.c_str(), &value, min, max);
		}
		else if (field.is_widget_type("drag")) {
			ImGui::DragInt(widget_name.c_str(), &value, (float)step, min, max);
		}
		else {
			ImGui::InputInt(widget_name.c_str(), &value, step);
		}
	}
	else if (type == typeid(glm::vec2)) {
		glm::vec2& value = *reinterpret_cast<glm::vec2*>(ptr);
		SHOW_FLOAT_FIELD(2, &value[0])
	}
	else if (type == typeid(glm::vec3)) {
		glm::vec3& value = *reinterpret_cast<glm::vec3*>(ptr);
		SHOW_FLOAT_FIELD_WITH_COLOR(3, &value[0])
	}
	else if (type == typeid(glm::vec4)) {
		glm::vec4& value = *reinterpret_cast<glm::vec4*>(ptr);
		SHOW_FLOAT_FIELD_WITH_COLOR(4, &value[0])
	}
	else if (type == typeid(std::string)) {
		std::string& value = *reinterpret_cast<std::string*>(ptr);
		static char str_buffer[256] = {};
		strcpy_s(str_buffer, value.c_str());
		if (ImGui::InputText(widget_name.c_str(), str_buffer, IM_ARRAYSIZE(str_buffer))) {
			value = std::string(str_buffer);
		}
	}
	else if (type == typeid(std::shared_ptr<Texture2D>)) {
		std::shared_ptr<Texture2D>& value = *reinterpret_cast<std::shared_ptr<Texture2D>*>(ptr);
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
	else if (type == typeid(std::shared_ptr<Animation>)) {
		std::shared_ptr<Animation>& value = *reinterpret_cast<std::shared_ptr<Animation>*>(ptr);
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
}

void show_type_field(void* instance, FieldInfo const& field) {
	bool const visible = (field.flag & FF_Visible) != 0;
	bool const editable = (field.flag & FF_Editable) != 0;
	if (!visible && !editable)
		return;

	void* ptr = (uint8_t*)instance + field.offset;

	if (!editable)
		ImGui::BeginDisabled();

	if (field.container) {
		if (ImGui::CollapsingHeader(field.name.c_str())) {
			ImGui::Indent();
			size_t size = field.container->size(ptr);

			if (!field.container->is_array && editable) {
				int new_size = (int)size;
				if (ImGui::InputInt("size", &new_size)) {
					if (new_size >= 0) {
						field.container->resize(ptr, new_size);
					}
				}
				size = field.container->size(ptr); // update size after resize
			}

			for (size_t i = 0; i < size; ++i) {
				void* elem_ptr = field.container->get(ptr, i);
				std::string elem_name = field.name + "[" + std::to_string(i) + "]";
				ImGui::Text(std::to_string(i).c_str());
				ImGui::SameLine();
				show_value(elem_ptr, *field.container->element_type, elem_name, field, field.container->element_enum_info);
			}
			ImGui::Unindent();
		}
	}
	else {
		ImGui::Text(field.name.c_str());
		show_value(ptr, *field.type, field.name, field, field.enum_info);
	}

	if (!editable)
		ImGui::EndDisabled();
}

void show_type_fields(void* instance, const std::string& name, bool group /*= false*/) {
	auto const info = TypeRegistry::instance().get(name);
	if (!info) return;

	if (group) {
		std::unordered_map<std::string, std::vector<const FieldInfo*>> groups;
		for (auto& field : info->fields) {
			std::string group_name = field.get_widget_value<std::string>("group", "other");
			groups[group_name].push_back(&field);
		}
		for (auto& [group_name, fields] : groups) {
			if (ImGui::CollapsingHeader(group_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Indent();
				for (auto& field : fields) {
					show_type_field(instance, *field);
				}
				ImGui::Unindent();
			}
		}
	}
	else {
		for (auto& field : info->fields) {
			show_type_field(instance, field);
		}
	}
}

#define SHOW_COMPONENT(ComponentType)                                               \
	if (ImGui::CollapsingHeader(#ComponentType, ImGuiTreeNodeFlags_DefaultOpen)) {  \
		auto& comp = selected_entity->get_component<ComponentType>();             \
		show_type_fields(&comp, TYPE_NAME(ComponentType));                          \
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

void show_properties(std::shared_ptr<Entity> const& selected_entity) {
	if (ImGui::Begin("properties")) {
		if (selected_entity) {
			SHOW_COMPONENT(TagComponent)
			SHOW_COMPONENT(TransformComponent)
			if (selected_entity->has_component<SkyLightComponent>()) {
				SHOW_COMPONENT(SkyLightComponent)
			}
			if (selected_entity->has_component<AnimationComponent>()) {
				SHOW_COMPONENT(AnimationComponent)
			}
			if (selected_entity->has_component<LightComponent>()) {
				SHOW_COMPONENT(LightComponent)
			}
			if (selected_entity->has_component<SpriteComponent>()) {
				SHOW_COMPONENT(SpriteComponent)
			}
			if (selected_entity->has_component<PostprocessVolumeComponent>()) {
				SHOW_COMPONENT(PostprocessVolumeComponent)
			}
			if (selected_entity->has_component<ParticleComponent>()) {
				SHOW_COMPONENT(ParticleComponent)
			}

			if (selected_entity->has_component<StaticMeshComponent>()) {
				if (ImGui::CollapsingHeader("StaticMeshComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
					auto& mesh = selected_entity->get_component<StaticMeshComponent>();
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

			if (selected_entity->has_component<SkeletalMeshComponent>()) {
				if (ImGui::CollapsingHeader("SkeletalMeshComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
					auto& mesh = selected_entity->get_component<SkeletalMeshComponent>();
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

			if (selected_entity->has_component<CameraComponent>()) {
				if (ImGui::CollapsingHeader("CameraComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
					auto& camera = selected_entity->get_component<CameraComponent>();
					show_type_fields(&camera, "CameraComponent");

					if (camera.m_is_perspective) {
						ImGui::InputFloat("field of view", &camera.m_intrinsic.fov, 0.01f);
					}
					else {
						ImGui::InputFloat("frustum size", &camera.m_intrinsic.size, 0.01f);
					}
					if (ImGui::RadioButton("is primary", camera.m_is_primary)) {
						if (!camera.m_is_primary) {
							g_runtime_context.m_scene->set_main_camera(selected_entity);
						}
					}
				}
			}

			if (selected_entity->has_component<ScriptComponent>()) {
				if (ImGui::CollapsingHeader("ScriptComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
					auto& script = selected_entity->get_component<ScriptComponent>();
					bool new_script_added = false;
					if (ImGui::BeginCombo("add", "select script...")) {
						auto const& metas = g_runtime_context.m_asset_manager->get_all_metas();
						for (auto const& meta : metas) {
							if (meta.type == "script") {
								if (ImGui::Selectable(meta.name().c_str())) {
									// Convert path to module name
									// e.g. scripts/test_mover.py -> scripts.test_mover
									std::string module_path = meta.path.generic_string();

									// remove extension
									size_t lastindex = module_path.find_last_of(".");
									if (lastindex != std::string::npos) {
										module_path = module_path.substr(0, lastindex);
									}

									// replace / with .
									std::replace(module_path.begin(), module_path.end(), '/', '.');
									// replace \ with .
									std::replace(module_path.begin(), module_path.end(), '\\', '.');

									// Assume class name is PascalCase(filename)
									// e.g. test_mover -> TestMover
									std::string stem = meta.path.stem().string();
									std::string class_name;
									bool next_upper = true;
									for (char c : stem) {
										if (c == '_') {
											next_upper = true;
										} else {
											if (next_upper) {
												class_name += toupper(c);
												next_upper = false;
											} else {
												class_name += c;
											}
										}
									}
									selected_entity->attach_script<PythonScript>(module_path, class_name);
									new_script_added = true;
								}
							}
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
				component_context_menu<CameraComponent>("camera component", selected_entity);
				component_context_menu<LightComponent>("light component", selected_entity);
				component_context_menu<SpriteComponent>("sprite component", selected_entity);
				component_context_menu<ScriptComponent>("script component", selected_entity, selected_entity);
				component_context_menu<SkyLightComponent>("skylight component", selected_entity);
				component_context_menu<AnimationComponent>("animation component", selected_entity);
				component_context_menu<PostprocessVolumeComponent>("postprocess volume component", selected_entity);
				component_context_menu<ParticleComponent>("particle component", selected_entity);
				ImGui::EndPopup();
			}
		}
	}
	ImGui::End();
}
