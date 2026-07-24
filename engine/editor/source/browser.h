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

		// Filters
		ImGui::Checkbox("Textures", &m_show_textures); ImGui::SameLine();
		ImGui::Checkbox("Materials", &m_show_materials); ImGui::SameLine();
		ImGui::Checkbox("Meshes", &m_show_meshes); ImGui::SameLine();
		ImGui::Checkbox("Others", &m_show_others);

		ImGui::Separator();

		// approximate width per item: text width + padding
		float max_item_width = 120.0f;
		int col_count = (int)(panel_width / max_item_width);
		if (col_count < 1) col_count = 1;

		ImGui::Columns(col_count, 0, false);

		for (auto& [_, child] : node->children) {
			if (!child->is_folder()) {
				std::string type = child->meta->type;
				if (type == "texture2d" && !m_show_textures) continue;
				else if ((type == "material" || type == "material instance") && !m_show_materials) continue;
				else if ((type == "static mesh" || type == "skeletal mesh") && !m_show_meshes) continue;
				else if (type != "texture2d" && type != "material" && type != "material instance" && type != "static mesh" && type != "skeletal mesh" && !m_show_others) continue;
			}

			ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			if (child->is_folder()) {
				color = ImVec4(0.9f, 0.8f, 0.2f, 1.0f); // Yellow for folders
			}
			else {
				std::string type = child->meta->type;
				if (type == "texture2d") color = ImVec4(1.0f, 0.5f, 0.5f, 1.0f); // Reddish for textures
				else if (type == "material" || type == "material instance") color = ImVec4(0.5f, 1.0f, 0.5f, 1.0f); // Greenish for materials
				else if (type == "static mesh" || type == "skeletal mesh") color = ImVec4(0.5f, 0.5f, 1.0f, 1.0f); // Blueish for meshes
				else if (type == "prefab") color = ImVec4(0.5f, 1.0f, 1.0f, 1.0f); // Cyan for prefabs
				else if (type == "scene") color = ImVec4(1.0f, 0.5f, 1.0f, 1.0f); // Magenta for scenes
			}

			ImGui::PushStyleColor(ImGuiCol_Text, color);

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
					if (ImGui::MenuItem("Rename")) {
						m_rename_target_guid = child->meta->guid;
						std::string filename = child->meta->path.filename().string();
						std::memset(m_rename_buffer, 0, sizeof(m_rename_buffer));
						std::strncpy(m_rename_buffer, filename.c_str(), sizeof(m_rename_buffer) - 1);
						m_rename_popup_open = true;
						ImGui::CloseCurrentPopup();
					}
					if (ImGui::MenuItem("Safe Delete")) {
						m_pending_delete_guid = child->meta->guid;
						m_pending_delete_label = child->name;
						m_delete_warnings = g_runtime_context.m_asset_manager->find_references(m_pending_delete_guid);
						m_delete_popup_open = true;
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

		if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
			if (ImGui::BeginMenu("Create")) {
				if (ImGui::MenuItem("Material Instance")) {
					m_create_mi_popup_open = true;
					std::strncpy(m_create_mi_name_buffer, "NewMaterialInstance", sizeof(m_create_mi_name_buffer));

					// Populate cached materials
					m_cached_materials = g_runtime_context.m_asset_manager->get_all_metas();
					// Filter only materials
					auto it = std::remove_if(m_cached_materials.begin(), m_cached_materials.end(), [](const AssetMeta& meta) {
						return meta.type != "material";
					});
					m_cached_materials.erase(it, m_cached_materials.end());
					m_create_mi_selected_mat_idx = -1;
				}
				ImGui::EndMenu();
			}
			ImGui::EndPopup();
		}

		ImGui::Columns(1);
	}

	void draw() {
		if (ImGui::Begin("browser")) {
			if (ImGui::Button("Import asset")) {
				std::string file = open_file_dialog();
				if (!file.empty()) {
					m_import_source_path = file;
					Filepath folder = get_curr_dir();
					Filepath dest = folder / Filepath(file).stem();
					std::memset(m_import_dest_buffer, 0, sizeof(m_import_dest_buffer));
					std::strncpy(m_import_dest_buffer, dest.generic_string().c_str(), sizeof(m_import_dest_buffer) - 1);
					m_import_popup_open = true;
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

			// --- Popups ---

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
				ImGui::EndPopup();
			}

			if (m_rename_popup_open) {
				ImGui::OpenPopup("Rename Asset");
				m_rename_popup_open = false;
			}
			if (ImGui::BeginPopupModal("Rename Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::InputText("New Name", m_rename_buffer, sizeof(m_rename_buffer));
				if (ImGui::Button("Cancel")) {
					ImGui::CloseCurrentPopup();
					m_rename_target_guid = {};
				}
				ImGui::SameLine();
				if (ImGui::Button("Rename")) {
					AssetMeta meta = g_runtime_context.m_asset_manager->get_meta(m_rename_target_guid);
					Filepath new_path = meta.path.parent_path() / m_rename_buffer;
					bool success = g_runtime_context.m_asset_manager->move_asset(m_rename_target_guid, new_path);
					m_action_status = success ? "Renamed asset" : "Failed to rename asset";
					m_action_success = success;
					ImGui::CloseCurrentPopup();
					m_rename_target_guid = {};
					m_selected_in_folder = nullptr;
				}
				ImGui::EndPopup();
			}

			if (m_create_mi_popup_open) {
				ImGui::OpenPopup("Create Material Instance");
				m_create_mi_popup_open = false;
			}
			if (ImGui::BeginPopupModal("Create Material Instance", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::InputText("Name", m_create_mi_name_buffer, sizeof(m_create_mi_name_buffer));

				std::string preview_value = "Select Material...";
				if (m_create_mi_selected_mat_idx >= 0 && m_create_mi_selected_mat_idx < m_cached_materials.size()) {
					preview_value = m_cached_materials[m_create_mi_selected_mat_idx].path.generic_string();
				}

				if (ImGui::BeginCombo("Parent Material", preview_value.c_str())) {
					for (int i = 0; i < m_cached_materials.size(); i++) {
						const bool is_selected = (m_create_mi_selected_mat_idx == i);
						if (ImGui::Selectable(m_cached_materials[i].path.generic_string().c_str(), is_selected)) {
							m_create_mi_selected_mat_idx = i;
						}
						if (is_selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				if (ImGui::Button("Cancel")) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Create")) {
					if (m_create_mi_selected_mat_idx >= 0) {
						Filepath folder = get_curr_dir();
						Filepath path = folder / m_create_mi_name_buffer;
						auto mat = Material::load(m_cached_materials[m_create_mi_selected_mat_idx].guid);
						if (mat) {
							auto mi = MaterialInstance::create(path, mat);
							if (mi) {
								m_action_status = "Created Material Instance";
								m_action_success = true;
							}
							else {
								m_action_status = "Failed to create Material Instance";
								m_action_success = false;
							}
						}
					}
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			if (m_import_popup_open) {
				ImGui::OpenPopup("Import Asset");
				m_import_popup_open = false;
			}
			if (ImGui::BeginPopupModal("Import Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Source: %s", m_import_source_path.c_str());
				ImGui::InputText("Destination Path", m_import_dest_buffer, sizeof(m_import_dest_buffer));

				if (ImGui::Button("Cancel")) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Import")) {
					if (import_asset(Filepath(m_import_source_path), Filepath(m_import_dest_buffer))) {
						m_import_status = "Import Successful";
						m_import_success = true;
					}
					else {
						// Error message is set in import_asset
						m_import_success = false;
					}
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			if (m_delete_popup_open) {
				ImGui::OpenPopup("Safe Delete Asset");
				m_delete_popup_open = false;
			}
			if (ImGui::BeginPopupModal("Safe Delete Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				if (!m_delete_warnings.empty()) {
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Warning: This asset is referenced by:");
					for (auto& ref : m_delete_warnings) {
						ImGui::Text("- %s", ref.generic_string().c_str());
					}
					ImGui::Separator();
				}

				ImGui::Text("Are you sure you want to delete %s?", m_pending_delete_label.c_str());

				if (ImGui::Button("Cancel")) {
					ImGui::CloseCurrentPopup();
					m_pending_delete_guid = {};
				}
				ImGui::SameLine();
				if (ImGui::Button("Delete")) {
					bool success = g_runtime_context.m_asset_manager->remove_asset(m_pending_delete_guid);
					m_action_status = success ? ("Deleted " + m_pending_delete_label) : "Failed to delete asset";
					m_action_success = success;
					m_pending_delete_guid = {};
					m_pending_delete_label.clear();
					m_selected_in_folder = nullptr;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
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

	bool import_asset(Filepath const& file, Filepath dest_path = {}) {
		if (file.empty()) {
			return false;
		}

		if (dest_path.empty()) {
			Filepath folder = get_curr_dir();
			dest_path = folder / file.stem();
		}

		auto root = FileSystem::get_root_path("");
		if (dest_path.is_absolute()) {
			// Try to make it relative to content root
			dest_path = std::filesystem::relative(dest_path, root);
		}

		Filepath folder = dest_path.parent_path();
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

	// Rename
	Guid m_rename_target_guid;
	bool m_rename_popup_open = false;
	char m_rename_buffer[260] = {};

	// Create Material Instance
	bool m_create_mi_popup_open = false;
	char m_create_mi_name_buffer[260] = "NewMaterialInstance";
	int m_create_mi_selected_mat_idx = -1;
	std::vector<AssetMeta> m_cached_materials;

	// Import
	bool m_import_popup_open = false;
	std::string m_import_source_path;
	char m_import_dest_buffer[260] = {};

	// Delete
	Guid m_pending_delete_guid;
	std::string m_pending_delete_label;
	bool m_delete_popup_open = false;
	std::vector<Filepath> m_delete_warnings;

	Guid m_move_target_guid;
	bool m_move_popup_open = false;
	char m_move_path_buffer[260] = {};

	std::function<void(AssetMeta* meta)> m_on_asset_opened;
	AssetNode* m_selected_in_hierachy = nullptr;
	AssetNode* m_selected_in_folder = nullptr;

	// Filters
	bool m_show_textures = true;
	bool m_show_materials = true;
	bool m_show_meshes = true;
	bool m_show_others = true;
};
