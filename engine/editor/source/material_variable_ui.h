#pragma once

#include "z1engine.h"
#include "imgui.h"
#include "asset/material.h"
#include "type_field.h"

using namespace z1;

// value widgets for one material variable; returns true on change
inline bool draw_material_variable(Material::Variable& var, std::string const& name, bool editable) {
	bool changed = false;
	auto& val = var.default_value;

	switch (var.type) {
	case DataType::Float:
		if (ImGui::InputFloat(("##" + name).c_str(), val.vec)) changed = true;
		break;
	case DataType::Float2:
		if (ImGui::InputFloat2(("##" + name).c_str(), val.vec)) changed = true;
		break;
	case DataType::Float3:
		if (ImGui::ColorEdit3(("##" + name).c_str(), val.vec)) changed = true;
		break;
	case DataType::Float4:
		if (ImGui::ColorEdit4(("##" + name).c_str(), val.vec)) changed = true;
		break;
	case DataType::Int:
		if (ImGui::InputInt(("##" + name).c_str(), val.ivec)) changed = true;
		break;
	case DataType::Int2:
		if (ImGui::InputInt2(("##" + name).c_str(), val.ivec)) changed = true;
		break;
	case DataType::Int3:
		if (ImGui::InputInt3(("##" + name).c_str(), val.ivec)) changed = true;
		break;
	case DataType::Int4:
		if (ImGui::InputInt4(("##" + name).c_str(), val.ivec)) changed = true;
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
		if (editable) {
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
		}
		break;
	default:
		ImGui::Text("unsupported type");
		break;
	}

	if (changed) {
		val.valid = true;
	}
	return changed;
}

// visible variables of a material or material instance; returns true on change
// overrides: draw valid-checkboxes per variable (material instances)
// editable: allow value edits and texture drag-drop
inline bool draw_material_variables(std::unordered_map<std::string, Material::Variable>& variables, bool overrides, bool editable) {
	bool changed = false;

	for (auto& [name, var] : variables) {
		if (!var.visible) continue;

		auto& val = var.default_value;
		ImGui::PushID(name.c_str());

		if (overrides) {
			if (ImGui::Checkbox(name.c_str(), &val.valid)) changed = true;
		}
		else {
			ImGui::Text("%s", name.c_str());
		}
		ImGui::Indent();

		bool disabled = (overrides && !val.valid) || !editable;
		if (disabled) {
			ImGui::BeginDisabled();
		}

		if (draw_material_variable(var, name, editable)) {
			changed = true;
		}

		if (disabled) {
			ImGui::EndDisabled();
		}
		ImGui::Unindent();
		ImGui::PopID();
	}

	return changed;
}
