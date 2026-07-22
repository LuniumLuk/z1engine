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

// Helper: assign an asset (loaded by guid) to a reflected shared_ptr field.
// shared_ptr_addr points to the raw memory of the std::shared_ptr<T>.
// Dispatches on AssetMeta::type to call the correct AssetType::load().
static void assign_asset_to_field(void* shared_ptr_addr, AssetMeta const& meta) {
	// We write into the shared_ptr memory directly.
	// std::shared_ptr layout: { T* ptr, control_block* ctrl }
	void** dst = static_cast<void**>(shared_ptr_addr);
	if (!dst) return;

	std::shared_ptr<void> loaded;

	if (meta.type == "static mesh") {
		auto asset = StaticMesh::load(meta.guid);
		dst[0] = asset.get();
		// Create a new shared_ptr in-place via placement new
		new (dst) std::shared_ptr<StaticMesh>(std::move(asset));
	} else if (meta.type == "skeletal mesh") {
		auto asset = SkeletalMesh::load(meta.guid);
		new (dst) std::shared_ptr<SkeletalMesh>(std::move(asset));
	} else if (meta.type == "texture2d") {
		auto asset = Texture2D::load(meta.guid);
		new (dst) std::shared_ptr<Texture2D>(std::move(asset));
	} else if (meta.type == "animation") {
		auto asset = Animation::load(meta.guid);
		new (dst) std::shared_ptr<Animation>(std::move(asset));
	} else if (meta.type == "skeleton") {
		auto asset = Skeleton::load(meta.guid);
		new (dst) std::shared_ptr<Skeleton>(std::move(asset));
	} else if (meta.type == "material") {
		auto asset = Material::load(meta.guid);
		new (dst) std::shared_ptr<Material>(std::move(asset));
	} else if (meta.type == "material instance") {
		auto asset = MaterialInstance::load(meta.guid);
		new (dst) std::shared_ptr<MaterialInstance>(std::move(asset));
	}
}

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

	if (field.is_guid) {
		Guid* value_ptr = reinterpret_cast<Guid*>(ptr);
		ImGui::Text("%s", value_ptr->is_valid() ? value_ptr->value.c_str() : "(empty)");
		return;
	}

	if (field.is_asset_ref) {
		// Generalized asset reference widget: display current asset, drag-drop, browse
		ImGui::Indent();

		std::string asset_type_hint = field.get_widget_value<std::string>("type", "");
		// Convert widget hint (e.g. "static_mesh") to asset meta type (e.g. "static mesh")
		std::string meta_type;
		for (char c : asset_type_hint) {
			if (c == '_') meta_type += ' ';
			else meta_type += c;
		}

		// Read current asset GUID via the shared_ptr's raw object pointer
		// AssetBase has virtual dtor → vtable ptr at offset 0; m_meta at sizeof(void*)
		void* shared_ptr_addr = ptr;
		void* const* raw_ptrs = static_cast<void* const*>(shared_ptr_addr);
		void* object_ptr = raw_ptrs ? raw_ptrs[0] : nullptr;

		if (object_ptr) {
			// Skip vtable pointer to reach AssetBase::m_meta
			uint8_t* meta_addr = static_cast<uint8_t*>(object_ptr) + sizeof(void*);
			auto* meta = reinterpret_cast<AssetMeta*>(meta_addr);
			ImGui::Text("%s", meta->guid.value.c_str());
			ImGui::SameLine();
			if (ImGui::Button("X")) {
				// Properly clear the shared_ptr using the registered clear callback
				if (field.clear_fn) {
					field.clear_fn(ptr);
				}
			}
		} else {
			ImGui::Text("(none)");
		}

		// "Select..." button to browse assets
		std::string popup_name = "Select Asset##" + name;
		if (ImGui::Button("Select...")) {
			ImGui::OpenPopup(popup_name.c_str());
		}

		// Browse popup: list all assets of matching type
		if (ImGui::BeginPopup(popup_name.c_str())) {
			auto const& metas = g_runtime_context.m_asset_manager->get_all_metas();
			for (auto const& meta : metas) {
				if (meta.type != meta_type) continue;

				std::string label = meta.path.generic_string() + " (" + meta.guid.value + ")";
				if (ImGui::Selectable(label.c_str())) {
					// Load the asset by type and assign to the shared_ptr
					assign_asset_to_field(shared_ptr_addr, meta);
				}
			}
			ImGui::EndPopup();
		}

		// Drag-drop target
		accept_payload("ASSET_ITEM", [&](void* data) {
			AssetMeta* meta = *(AssetMeta**)data;
			if (meta->type == meta_type) {
				assign_asset_to_field(shared_ptr_addr, *meta);
			}
		});

		ImGui::Unindent();
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

// Forward declaration
static void show_type_fields_for_entity(std::shared_ptr<Entity> const& entity, std::string const& type_name);

void show_properties(std::shared_ptr<Entity> const& selected_entity) {
	if (ImGui::Begin("properties")) {
		if (selected_entity) {
			// Always show TagComponent and TransformComponent first
			if (ImGui::CollapsingHeader("TagComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
				auto& comp = selected_entity->get_component<TagComponent>();
				show_type_fields(&comp, "TagComponent");
			}
			if (ImGui::CollapsingHeader("TransformComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
				auto& comp = selected_entity->get_component<TransformComponent>();
				show_type_fields(&comp, "TransformComponent");
			}

			// Show all other components via reflection
			auto components = TypeRegistry::instance().get_all_components();
			for (auto* info : components) {
				if (info->name == "TagComponent" || info->name == "TransformComponent")
					continue;

				// Check if entity has this component using the has_in hook
				if (info->has_in && info->has_in(*selected_entity)) {
					if (ImGui::CollapsingHeader(info->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
						// For ScriptComponent, show script entries widget
						if (info->name == "ScriptComponent") {
							auto& script = selected_entity->get_component<ScriptComponent>();
							if (ImGui::CollapsingHeader("script_entries", ImGuiTreeNodeFlags_DefaultOpen)) {
								ImGui::Indent();
								for (size_t i = 0; i < script.m_scripts.size(); ++i) {
									auto& sd = script.m_scripts[i];
									if (sd.instance) {
										ImGui::Text("%s", sd.instance->get_script_name().c_str());
									}
								}
								ImGui::Unindent();
							}
							// Add script button
							if (ImGui::BeginCombo("add script", "select script...")) {
								auto const& metas = g_runtime_context.m_asset_manager->get_all_metas();
								for (auto const& meta : metas) {
									if (meta.type == "script") {
										std::string name = meta.path.stem().string();
										if (ImGui::Selectable(name.c_str())) {
											selected_entity->attach_script<PythonScript>(name, "Script");
										}
									}
								}
								ImGui::EndCombo();
							}
						} else {
							// Generic: get component and show fields
							// Use the construct hook to get a typed pointer
							// Since we can't easily get a void* from an entity component generically,
							// we need to special-case each component or add a get_component_void API
							// For now, fall back to per-type handling for those we know
							show_type_fields_for_entity(selected_entity, info->name);
						}
					}
				}
			}

			// CameraComponent needs special handling for the m_intrinsic union (fov/size)
			if (selected_entity->has_component<CameraComponent>()) {
				if (ImGui::CollapsingHeader("CameraComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
					auto& camera = selected_entity->get_component<CameraComponent>();
					show_type_fields(&camera, "CameraComponent");

					if (camera.m_is_perspective) {
						ImGui::InputFloat("field of view", &camera.m_intrinsic.fov, 0.01f);
					} else {
						ImGui::InputFloat("frustum size", &camera.m_intrinsic.size, 0.01f);
					}
					if (ImGui::RadioButton("is primary", camera.m_is_primary)) {
						if (!camera.m_is_primary) {
							g_runtime_context.m_scene->set_main_camera(selected_entity);
						}
					}
				}
			}
		}
		show_properties_context_menu(selected_entity);
	}
	ImGui::End();
}

// Helper: display type fields for an entity's component by name
static void show_type_fields_for_entity(std::shared_ptr<Entity> const& entity, std::string const& type_name) {
#define SHOW_IF_COMPONENT(T) if (type_name == #T) { auto& comp = entity->get_component<T>(); show_type_fields(&comp, #T); return; }
	SHOW_IF_COMPONENT(LightComponent)
	SHOW_IF_COMPONENT(SpriteComponent)
	SHOW_IF_COMPONENT(SkyLightComponent)
	SHOW_IF_COMPONENT(AnimationComponent)
	SHOW_IF_COMPONENT(ParticleComponent)
	SHOW_IF_COMPONENT(PostprocessVolumeComponent)
	SHOW_IF_COMPONENT(StaticMeshComponent)
	SHOW_IF_COMPONENT(SkeletalMeshComponent)
#undef SHOW_IF_COMPONENT
}

void show_properties_context_menu(std::shared_ptr<Entity> const& selected_entity) {
	if (!selected_entity) return;

	if (ImGui::BeginPopupContextWindow()) {
		// Generate add/remove component menu from TypeRegistry
		auto components = TypeRegistry::instance().get_all_components();
		for (auto* info : components) {
			if (info->name == "TagComponent" || info->name == "TransformComponent")
				continue;

			if (info->has_in && info->has_in(*selected_entity)) {
				std::string label = "remove " + info->name;
				if (ImGui::MenuItem(label.c_str())) {
					if (info->remove_from)
						info->remove_from(*selected_entity);
				}
			} else {
				std::string label = "add " + info->name;
				if (ImGui::MenuItem(label.c_str())) {
					if (info->add_to)
						info->add_to(*selected_entity);
				}
			}
		}
		ImGui::EndPopup();
	}
}
