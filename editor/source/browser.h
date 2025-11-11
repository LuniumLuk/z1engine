#pragma once

#include "z1engine.h"
#include "imgui.h"

using namespace z1;

struct ContentBrowser {

	void draw_asset_node(AssetNode* node) {
		if (!node || !node->is_folder())
			return;

		if (!node->parent) {
			for (auto& [_, child] : node->children) {
				draw_asset_node(child.get());
			}
			return;
		}

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_OpenOnArrow;

		if (!node->has_subfolder())
			flags |= ImGuiTreeNodeFlags_Leaf; // no subfolder

		if (!node->parent)
			flags |= ImGuiTreeNodeFlags_DefaultOpen; // root folder is open by default

		auto* n = m_selected_in_hierachy;
		while (n) {
			if (n->parent == node) {
				ImGui::SetNextItemOpen(true);
				break;
			}
			n = n->parent;
		}

		if (m_selected_in_hierachy == node)
			flags |= ImGuiTreeNodeFlags_Selected; // highlight selected folder

		bool opened = ImGui::TreeNodeEx(node->name.c_str(), flags);

		// handle selection
		if (ImGui::IsItemClicked()) {
			m_selected_in_hierachy = node;
			m_selected_in_folder = nullptr; // clear selection in folder view
		}

		if (opened) {
			for (auto& [_, child] : node->children) {
				draw_asset_node(child.get());
			}
			ImGui::TreePop();
		}
	}

	void draw_folder_view(AssetNode* node) {
		float const padding = 16.0f;
		float panel_width = ImGui::GetContentRegionAvail().x;

		// approximate width per item: text width + padding
		float max_item_width = 120.0f;
		int col_count = (int)(panel_width / max_item_width);
		if (col_count < 1) col_count = 1;

		ImGui::Columns(col_count, 0, false);

		for (auto& [_, child] : node->children) {
			ImGui::PushStyleColor(ImGuiCol_Text,
				child->is_folder()
				? ImVec4(0.9f, 0.8f, 0.2f, 1.0f)  // yellow for folders
				: ImVec4(1.0f, 1.0f, 1.0f, 1.0f)  // white for assets
			);

			bool selected = (m_selected_in_folder == child.get());
			// handle single-click
			if (ImGui::Selectable(child->name.c_str(), selected)) {
				m_selected_in_folder = child.get();
			}

			// start drag source
			if (ImGui::BeginDragDropSource()) {
				ImGui::SetDragDropPayload("ASSET_ITEM", &child->meta, sizeof(AssetMeta*));
				ImGui::Text("%s", child->name.c_str());
				ImGui::EndDragDropSource();
			}

			// handle double-click
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
				if (child->is_folder()) {
					m_selected_in_hierachy = child.get();
					m_selected_in_folder = nullptr;
				}
				else {
					if (m_on_asset_opened) {
						m_on_asset_opened(child->meta);
					}
				}
			}

			// handle right-click
			if (ImGui::BeginPopupContextItem(child->name.c_str())) {
				if (ImGui::MenuItem("copy guid")) {
					ImGui::SetClipboardText(child->meta->guid.value.c_str());
				}
				if (ImGui::MenuItem("copy path")) {
					ImGui::SetClipboardText(child->meta->path.generic_string().c_str());
				}
				ImGui::EndPopup();
			}

			// show tooltip for assets
			if (ImGui::IsItemHovered() && !child->is_folder()) {
				ImGui::BeginTooltip();
				ImGui::Text("guid: %s", child->meta->guid.value.c_str());
				ImGui::Text("type: %s", child->meta->type.c_str());
				ImGui::Text("path: %s", child->meta->path.generic_string().c_str());
				ImGui::EndTooltip();
			}

			ImGui::PopStyleColor();
			ImGui::NextColumn();
		}

		ImGui::Columns(1);
	}

	void draw() {
		if (ImGui::Begin("browser")) {
			if (ImGui::BeginTable("content browser table", 2, ImGuiTableFlags_Resizable)) {
				// Left column: hierarchy
				ImGui::TableNextColumn();
				ImGui::BeginChild("hierarchy panel");
				{
					AssetNode* root = g_runtime_context.m_asset_manager->get_asset_tree_root();
					if (root) {
						draw_asset_node(root);
					}
				}
				ImGui::EndChild();

				// Right column: folder view
				ImGui::TableNextColumn();
				ImGui::BeginChild("folder view panel");
				{
					if (m_selected_in_hierachy) {
						draw_folder_view(m_selected_in_hierachy);
					}
				}
				ImGui::EndChild();

				ImGui::EndTable();
			}
		}
		ImGui::End();
	}

	Filepath get_curr_dir() const {
		if (m_selected_in_hierachy) {
			// build path from root to selected folder
			std::vector<std::string> parts;
			AssetNode* node = m_selected_in_hierachy;
			while (node && !node->is_root()) {
				parts.push_back(node->name);
				node = node->parent;
			}
			std::reverse(parts.begin(), parts.end());
			Filepath path = "";
			for (auto& part : parts) {
				path /= part;
			}
			return path;
		}
		return "";
	}

	std::function<void(AssetMeta* meta)> m_on_asset_opened;
	AssetNode* m_selected_in_hierachy = nullptr;
	AssetNode* m_selected_in_folder = nullptr;
};
