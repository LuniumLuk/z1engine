#pragma once

#include <filesystem>
#include <cstring>

#include "z1engine.h"
#include "gui.h"
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
				if (!child->is_folder()) {
					if (ImGui::MenuItem("copy guid")) {
						ImGui::SetClipboardText(child->meta->guid.value.c_str());
					}
					if (ImGui::MenuItem("copy path")) {
						ImGui::SetClipboardText(child->meta->path.generic_string().c_str());
					}
					if (ImGui::MenuItem("Delete asset")) {
						m_pending_delete_guid = child->meta->guid;
						m_pending_delete_label = child->name;
						m_selected_in_folder = nullptr;
						ImGui::CloseCurrentPopup();
					}
					if (ImGui::MenuItem("Move asset")) {
						m_move_target_guid = child->meta->guid;
						std::memset(m_move_path_buffer, 0, sizeof(m_move_path_buffer));
						std::strncpy(m_move_path_buffer, child->meta->path.generic_string().c_str(), sizeof(m_move_path_buffer) - 1);
						m_move_popup_open = true;
						ImGui::CloseCurrentPopup();
					}
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
			if (ImGui::Button("Import asset")) {
				static const char import_filter[] =
					"Textures (*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.psd;*.gif;*.pic;*.exr)\0"
					"*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.psd;*.gif;*.pic;*.exr\0"
					"Models (*.obj;*.gltf;*.glb)\0"
					"*.obj;*.gltf;*.glb\0"
					"All Files\0*.*\0";
				std::string file = open_file_dialog(import_filter);
				if (!file.empty()) {
					import_asset(Filepath(file));
				}
			}
			if (!m_import_status.empty()) {
				ImVec4 color = m_import_success
					? ImVec4(0.2f, 0.9f, 0.3f, 1.0f)
					: ImVec4(0.9f, 0.3f, 0.2f, 1.0f);
				ImGui::TextColored(color, "%s", m_import_status.c_str());
			}
			if (!m_action_status.empty()) {
				ImVec4 color = m_action_success
					? ImVec4(0.2f, 0.9f, 0.3f, 1.0f)
					: ImVec4(0.9f, 0.3f, 0.2f, 1.0f);
				ImGui::TextColored(color, "%s", m_action_status.c_str());
			}
			ImGui::Separator();
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
			if (m_move_popup_open) {
				ImGui::OpenPopup("Move Asset");
				m_move_popup_open = false;
			}
			if (ImGui::BeginPopupModal("Move Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				if (m_move_target_guid.is_valid()) {
					ImGui::InputText("New path", m_move_path_buffer, sizeof(m_move_path_buffer));
					if (ImGui::Button("Cancel")) {
						ImGui::CloseCurrentPopup();
						m_move_target_guid = {};
					}
					ImGui::SameLine();
					if (ImGui::Button("Move")) {
						Filepath dest = m_move_path_buffer;
						if (dest.empty()) {
							m_action_status = "Move path cannot be empty";
							m_action_success = false;
						}
						else {
							bool success = g_runtime_context.m_asset_manager->move_asset(m_move_target_guid, dest);
							m_action_status = success ? std::string("Moved asset to ") + m_move_path_buffer : "Failed to move asset";
							m_action_success = success;
						}
						ImGui::CloseCurrentPopup();
						m_move_target_guid = {};
						m_selected_in_folder = nullptr;
					}
				}
				else {
					ImGui::Text("No asset selected");
					if (ImGui::Button("Close")) {
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::EndPopup();
			}
			if (m_pending_delete_guid.is_valid()) {
				bool success = g_runtime_context.m_asset_manager->remove_asset(m_pending_delete_guid);
				m_action_status = success ? ("Deleted " + m_pending_delete_label) : "Failed to delete asset";
				m_action_success = success;
				m_pending_delete_guid = {};
				m_pending_delete_label.clear();
				m_selected_in_folder = nullptr;
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

	bool import_asset(Filepath const& file) {
		if (file.empty()) {
			return false;
		}

		Filepath folder = get_curr_dir();
		Filepath dest_path = folder / file.stem();
		auto const& root = FileSystem::s_content_root;
		if (!folder.empty()) {
			try {
				std::filesystem::create_directories(root / folder);
			}
			catch (...) {
			}
		}

		ImportResult result{};
		if (TextureImporter::can_import(file)) {
			TextureImporterSettings settings{};
			settings.file = file;
			settings.path = dest_path;
			result = TextureImporter::import(settings);
		}
		else if (ObjImporter::can_import(file)) {
			ObjImporterSettings settings{};
			settings.file = file;
			settings.path = dest_path;
			result = ObjImporter::import(settings);
		}
		else if (GltfImporter::can_import(file)) {
			GltfImporterSettings settings{};
			settings.file = file;
			settings.path = dest_path;
			result = GltfImporter::import(settings);
		}
		else {
			m_import_status = "Unsupported file type: " + file.extension().string();
			m_import_success = false;
			return false;
		}

		if (result.success) {
			std::vector<std::string> asset_paths;
			for (auto const& meta : result.assets) {
				asset_paths.push_back(meta.path.generic_string());
			}
			std::string display = join_strings(asset_paths);
			if (display.empty()) {
				display = file.filename().string();
			}
			m_import_status = "Imported " + display;
			if (!result.warnings.empty()) {
				m_import_status += " (warnings: " + join_strings(result.warnings) + ")";
			}
			m_import_success = true;
			return true;
		}

		std::string error_message = join_strings(result.errors);
		if (error_message.empty()) {
			error_message = "Import failed";
		}
		m_import_status = error_message;
		m_import_success = false;
		return false;
	}

	static std::string join_strings(std::vector<std::string> const& entries) {
		std::string output;
		for (size_t i = 0; i < entries.size(); ++i) {
			if (i > 0) {
				output += ", ";
			}
			output += entries[i];
		}
		return output;
	}

	std::string m_import_status;
	bool m_import_success = false;

	std::string m_action_status;
	bool m_action_success = false;
	Guid m_move_target_guid;
	bool m_move_popup_open = false;
	char m_move_path_buffer[260] = {};
	Guid m_pending_delete_guid;
	std::string m_pending_delete_label;

	std::function<void(AssetMeta* meta)> m_on_asset_opened;
	AssetNode* m_selected_in_hierachy = nullptr;
	AssetNode* m_selected_in_folder = nullptr;
};
