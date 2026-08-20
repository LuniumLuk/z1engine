#pragma once

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "z1engine.h"
#include "imgui.h"
#include "asset/material.h"
#include "asset/mesh.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "scene/component/camera.h"
#include "scene/component/light.h"
#include "render/framebuffer.h"
#include "render/renderer/renderer_forward.h"
#include "render/renderer/renderer_deferred.h"
#include "type_field.h"

using namespace z1;

// Edit window for material and material instance assets.
// Owns a preview scene (sphere + lights) rendered with the active renderer.
struct MaterialEditor {

	bool is_open() const { return m_open; }

	void open(AssetMeta* meta);
	void close();

	// render the preview scene, called before the main viewport draw each frame
	// the preview framebuffer has a fixed size (512x512) independent of the viewport
	void render_preview(float delta_time);

	// draw the imgui window, called from on_imgui_render
	void draw();

private:
	static std::shared_ptr<StaticMesh> create_sphere_mesh();
	void build_preview_scene();
	void ensure_renderer();
	void update_camera();
	bool draw_variable_widgets(std::unordered_map<std::string, Material::Variable>& variables, bool overrides);
	bool draw_material_fields();
	bool draw_instance_fields();
	void save();

	bool m_open = false;
	bool m_is_instance = false;
	AssetMeta m_meta = {};

	// edited assets
	std::shared_ptr<Material> m_material;
	std::shared_ptr<MaterialInstance> m_instance;
	std::shared_ptr<MaterialInstance> m_preview_instance;

	// preview scene
	std::shared_ptr<Scene> m_scene;
	std::shared_ptr<Entity> m_camera_entity;
	std::shared_ptr<Framebuffer> m_preview_fb;

	// dedicated renderer so preview rendering never disturbs the main viewport state
	bool m_renderer_initialized = false;
	RenderMode m_renderer_mode = RenderMode::Forward;
	std::shared_ptr<RendererForward> m_renderer_forward;
	std::shared_ptr<RendererDeferred> m_renderer_deferred;

	// orbit controls
	float m_yaw = 0.0f;
	float m_pitch = 0.25f;
	float m_distance = 2.8f;
	bool m_auto_rotate = true;

	bool m_preview_hovered = false;
};

inline std::shared_ptr<StaticMesh> MaterialEditor::create_sphere_mesh() {
	constexpr int stacks = 24;
	constexpr int slices = 32;
	constexpr float pi = 3.14159265359f;

	std::vector<StaticMesh::VertexData> vertices;
	std::vector<uint32_t> indices;
	vertices.reserve((stacks + 1) * (slices + 1));
	indices.reserve(stacks * slices * 6);

	for (int i = 0; i <= stacks; ++i) {
		float phi = pi * i / stacks;
		float v = (float)i / stacks;
		for (int j = 0; j <= slices; ++j) {
			float theta = 2.0f * pi * j / slices;
			float u = (float)j / slices;

			glm::vec3 p = {
				std::sin(phi) * std::cos(theta),
				std::cos(phi),
				std::sin(phi) * std::sin(theta)
			};
			glm::vec3 tangent = glm::normalize(glm::vec3{ std::cos(theta), 0.0f, -std::sin(theta) });

			StaticMesh::VertexData vd;
			vd.position = p;
			vd.normal = p;
			vd.texcoord0 = { u, v };
			vd.tangent = glm::vec4{ tangent, 1.0f };
			vertices.push_back(vd);
		}
	}

	for (int i = 0; i < stacks; ++i) {
		for (int j = 0; j < slices; ++j) {
			uint32_t a = i * (slices + 1) + j;
			uint32_t b = a + slices + 1;
			indices.push_back(a);
			indices.push_back(a + 1);
			indices.push_back(b);
			indices.push_back(b);
			indices.push_back(a + 1);
			indices.push_back(b + 1);
		}
	}

	return std::make_shared<StaticMesh>(vertices, indices, PrimitiveType::Triangles);
}

inline void MaterialEditor::open(AssetMeta* meta) {
	if (!meta) return;
	close();

	m_meta = *meta;
	m_is_instance = (meta->type == "material instance");

	if (m_is_instance) {
		m_instance = g_runtime_context.m_asset_manager->get<MaterialInstance>(meta->guid);
		if (!m_instance) {
			CORE_WARN("material editor: failed to load material instance {0}", meta->guid);
			return;
		}
		m_material = m_instance->m_material;
		m_preview_instance = m_instance;
	}
	else {
		m_material = g_runtime_context.m_asset_manager->get<Material>(meta->guid);
		if (!m_material) {
			CORE_WARN("material editor: failed to load material {0}", meta->guid);
			return;
		}
		// wrap the material so the renderer can bind it like a material instance
		m_preview_instance = std::make_shared<MaterialInstance>(m_material);
	}

	build_preview_scene();
	m_open = true;
}

inline void MaterialEditor::close() {
	m_open = false;
	m_preview_hovered = false;
	m_preview_instance.reset();
	m_instance.reset();
	m_material.reset();
	m_scene.reset();
	m_camera_entity.reset();
	m_preview_fb.reset();
	m_renderer_deferred.reset();
	m_renderer_forward.reset();
	m_renderer_initialized = false;
}

inline void MaterialEditor::build_preview_scene() {
	m_scene = std::make_shared<Scene>();

	// camera
	m_camera_entity = m_scene->create_transient_entity("[Material Editor] Camera");
	m_camera_entity->add_component<CameraComponent>();
	m_scene->set_main_camera(m_camera_entity);
	update_camera();

	// key light
	{
		auto light = m_scene->create_entity("[Material Editor] Key Light");
		light->add_component<LightComponent>(LightType::Directional, glm::vec3(1.0f), 3.0f);
		light->get_component<TransformComponent>().m_rotation = { 45.0f, -30.0f, 0.0f };
	}

	// fill light
	{
		auto light = m_scene->create_entity("[Material Editor] Fill Light");
		light->add_component<LightComponent>(LightType::Directional, glm::vec3(1.0f), 1.0f);
		light->get_component<TransformComponent>().m_rotation = { -20.0f, 150.0f, 0.0f };
	}

	// preview sphere
	{
		auto sphere = m_scene->create_entity("[Material Editor] Sphere");
		auto& mesh_comp = sphere->add_component<StaticMeshComponent>(create_sphere_mesh());
		mesh_comp.m_override_materials["slot0"] = m_preview_instance;
	}
}

inline void MaterialEditor::ensure_renderer() {
	auto mode = g_runtime_context.m_global->render_mode;
	if (m_renderer_initialized && mode == m_renderer_mode) return;

	m_renderer_deferred.reset();
	m_renderer_forward.reset();
	m_renderer_mode = mode;
	if (mode == RenderMode::Deferred) {
		m_renderer_deferred = std::make_shared<RendererDeferred>();
	}
	else {
		m_renderer_forward = std::make_shared<RendererForward>();
	}
	m_renderer_initialized = true;
}

inline void MaterialEditor::update_camera() {
	if (!m_camera_entity) return;

	m_pitch = std::clamp(m_pitch, -1.55f, 1.55f);
	m_distance = std::clamp(m_distance, 1.5f, 20.0f);

	auto& trans = m_camera_entity->get_component<TransformComponent>();
	trans.m_location = {
		m_distance * std::sin(m_yaw) * std::cos(m_pitch),
		-m_distance * std::sin(m_pitch),
		m_distance * std::cos(m_yaw) * std::cos(m_pitch)
	};
	trans.m_rotation = { glm::degrees(m_pitch), glm::degrees(m_yaw), 0.0f };
}

inline void MaterialEditor::render_preview(float delta_time) {
	if (!m_open || !m_scene) return;

	constexpr uint32_t kPreviewSize = 512;
	if (!m_preview_fb) {
		m_preview_fb = Framebuffer::create(kPreviewSize, kPreviewSize, {
			{ ImageFormat::RGBA8, SamplerMode::Nearest, WrapMode::ClampToBorder },
			{ ImageFormat::DepthStencil },
		});
	}

	ensure_renderer();

	if (m_auto_rotate) {
		m_yaw += 0.3f * delta_time;
	}
	update_camera();

	if (m_renderer_mode == RenderMode::Deferred && m_renderer_deferred) {
		m_renderer_deferred->draw(m_scene, m_preview_fb);
	}
	else if (m_renderer_forward) {
		m_renderer_forward->draw(m_scene, m_preview_fb);
	}

	// store transforms so the next frame's velocity/TAA history is correct
	m_scene->on_update(0.0f);
}

inline bool MaterialEditor::draw_variable_widgets(std::unordered_map<std::string, Material::Variable>& variables, bool overrides) {
	bool changed = false;

	for (auto& [name, var] : variables) {
		if (!var.visible) continue;

		auto& val = var.default_value;
		ImGui::PushID(name.c_str());

		if (overrides) {
			ImGui::Checkbox(name.c_str(), &val.valid);
			ImGui::Indent();
			if (!val.valid) {
				ImGui::BeginDisabled();
			}
		}
		else {
			ImGui::Text("%s", name.c_str());
			ImGui::Indent();
		}

		switch (var.type) {
		case DataType::Float:
			if (ImGui::InputFloat(("##" + name).c_str(), val.vec)) { val.valid = true; changed = true; }
			break;
		case DataType::Float2:
			if (ImGui::InputFloat2(("##" + name).c_str(), val.vec)) { val.valid = true; changed = true; }
			break;
		case DataType::Float3:
			if (ImGui::ColorEdit3(("##" + name).c_str(), val.vec)) { val.valid = true; changed = true; }
			break;
		case DataType::Float4:
			if (ImGui::ColorEdit4(("##" + name).c_str(), val.vec)) { val.valid = true; changed = true; }
			break;
		case DataType::Int:
			if (ImGui::InputInt(("##" + name).c_str(), val.ivec)) { val.valid = true; changed = true; }
			break;
		case DataType::Int2:
			if (ImGui::InputInt2(("##" + name).c_str(), val.ivec)) { val.valid = true; changed = true; }
			break;
		case DataType::Int3:
			if (ImGui::InputInt3(("##" + name).c_str(), val.ivec)) { val.valid = true; changed = true; }
			break;
		case DataType::Int4:
			if (ImGui::InputInt4(("##" + name).c_str(), val.ivec)) { val.valid = true; changed = true; }
			break;
		case DataType::Sampler2D:
			if (val.tex2D) {
				auto const& texture = val.tex2D;
				auto w = texture->m_image->get_description().m_width;
				auto h = texture->m_image->get_description().m_height;
				ImGui::Image(texture->m_image->get_native_handle(), ImVec2(64.0f * w / h, 64.0f), ImVec2(0, 1), ImVec2(1, 0));
			}
			else {
				ImGui::Text("(no texture)");
			}
			accept_payload("ASSET_ITEM",
				[&](void* data) {
					AssetMeta* dropped = *(AssetMeta**)data;
					if (dropped && dropped->type == "texture2d") {
						val.tex2D = Asset<Texture2D>::load(dropped->guid);
						val.type = var.type;
						val.valid = true;
						changed = true;
					}
				}
			);
			break;
		default:
			ImGui::Text("unsupported type");
			break;
		}

		if (overrides && !val.valid) {
			ImGui::EndDisabled();
		}
		ImGui::Unindent();
		ImGui::PopID();
	}

	return changed;
}

inline bool MaterialEditor::draw_material_fields() {
	bool changed = false;
	uint32_t flags = m_material->m_flags;

	int alpha_mode = (int)MaterialFlags::get_alpha_mode(flags);
	if (ImGui::Combo("alpha mode", &alpha_mode, "Opaque\0Mask\0Blend\0")) {
		MaterialFlags::set_alpha_mode(flags, static_cast<AlphaMode>(alpha_mode));
		changed = true;
	}

	int cull_mode = (int)MaterialFlags::get_cull_mode(flags);
	if (ImGui::Combo("cull mode", &cull_mode, "None\0Front\0Back\0Front And Back\0")) {
		MaterialFlags::set_cull_mode(flags, static_cast<CullMode>(cull_mode));
		changed = true;
	}

	bool depth_test = MaterialFlags::get_depth_test(flags);
	if (ImGui::Checkbox("depth test", &depth_test)) {
		MaterialFlags::set_depth_test(flags, depth_test);
		changed = true;
	}

	bool depth_write = MaterialFlags::get_depth_write(flags);
	if (ImGui::Checkbox("depth write", &depth_write)) {
		MaterialFlags::set_depth_write(flags, depth_write);
		changed = true;
	}

	if (changed) {
		m_material->m_flags = flags;
	}

	if (draw_variable_widgets(m_material->m_variables, false)) {
		changed = true;
	}

	return changed;
}

inline bool MaterialEditor::draw_instance_fields() {
	ImGui::Text("parent material: %s", m_material->m_meta.path.generic_string().c_str());
	ImGui::Text("shader: %s", m_material->get_shader()->get_name().c_str());
	ImGui::Separator();
	return draw_variable_widgets(m_instance->m_override_variables, true);
}

inline void MaterialEditor::save() {
	if (m_is_instance) {
		if (m_instance) {
			m_instance->save();
			m_instance->mark_saved();
		}
	}
	else {
		if (m_material) {
			m_material->save();
			m_material->mark_saved();
		}
	}
}

inline void MaterialEditor::draw() {
	if (!m_open) return;

	std::string title = m_is_instance ? "Material Instance Editor" : "Material Editor";
	title += " - " + m_meta.name();

	bool open = true;
	ImGui::Begin((title + "##" + m_meta.guid.value).c_str(), &open,
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking);
	if (!open) {
		ImGui::End();
		close();
		return;
	}

	// two column layout: left = preview (not scrollable), right = attributes (scrollable)
	auto avail = ImGui::GetContentRegionAvail();
	float left_width = std::min(512.0f, std::max(220.0f, avail.x * 0.45f));

	ImGui::BeginChild("##preview_panel", ImVec2(left_width, 0), false,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	{
		m_preview_hovered = false;
		if (m_preview_fb) {
			auto w = (float)m_preview_fb->get_width();
			auto h = (float)m_preview_fb->get_height();
			auto region = ImGui::GetContentRegionAvail();
			region.y -= ImGui::GetFrameHeightWithSpacing() * 2.0f; // room for the controls row

			ImVec2 present_size;
			if (region.y / region.x > h / w) {
				present_size.x = region.x;
				present_size.y = region.x * h / w;
			}
			else {
				present_size.x = region.y * w / h;
				present_size.y = region.y;
			}

			ImGui::BeginChild("preview", present_size, false, ImGuiWindowFlags_NoScrollbar);
			ImVec2 image_pos = ImGui::GetCursorScreenPos();
			ImGui::Image(m_preview_fb->get_attachment_native_handle(0), present_size, ImVec2(0, 1), ImVec2(1, 0));

			// interactive canvas over the image: prevents imgui from treating the
			// area as window background (move/scroll) and captures the mouse so the
			// camera can use io deltas, which work in every imgui viewport window
			// (engine mouse events only see the main window)
			ImGui::SetCursorScreenPos(image_pos);
			ImGui::InvisibleButton("##preview_canvas", present_size,
				ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
			m_preview_hovered = ImGui::IsItemHovered();

			if (m_preview_hovered || ImGui::IsItemActive()) {
				auto& io = ImGui::GetIO();
				if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
					m_yaw -= io.MouseDelta.x * 0.01f;
					m_pitch += io.MouseDelta.y * 0.01f;
					update_camera();
				}
				if (m_preview_hovered && io.MouseWheel != 0.0f) {
					m_distance *= (1.0f - io.MouseWheel * 0.1f);
					update_camera();
				}
				io.WantCaptureMouse = true;
			}
			ImGui::EndChild();
		}

		ImGui::Checkbox("auto rotate", &m_auto_rotate);
		ImGui::Text("drag to orbit, scroll to zoom");
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// right column: attributes (scrollable)
	ImGui::BeginChild("##attributes_panel", ImVec2(0, 0), false);
	{
		// save bar
		if (ImGui::Button("Save")) {
			save();
		}
		ImGui::SameLine();
		bool dirty = m_is_instance ? (m_instance && m_instance->m_is_dirty) : (m_material && m_material->m_is_dirty);
		ImGui::TextColored(dirty ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%s", dirty ? "unsaved changes" : "saved");

		ImGui::Separator();

		// fields
		bool changed = m_is_instance ? draw_instance_fields() : draw_material_fields();
		if (changed) {
			if (m_is_instance && m_instance) m_instance->mark_dirty();
			else if (m_material) m_material->mark_dirty();
		}
	}
	ImGui::EndChild();

	ImGui::End();
}
