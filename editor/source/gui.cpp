#include "z1engine.h"
#include "gui.h"

#include <windows.h>
#include <commdlg.h>

using namespace z1;

// https://cplusplus.com/forum/windows/74644/#msg400037
static std::string wstrtostr(const std::wstring& wstr) {
	std::string str;
	char* sz = new char[wstr.length() + 1];
	sz[wstr.size()] = '\0';
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, sz, (int)wstr.length(), NULL, NULL);
	str = sz;
	delete[] sz;
	return str;
}

static void strtowstr(std::string const& str, wchar_t* wstr) {
	mbstowcs(wstr, str.c_str(), str.size() + 1);
}

static std::string open_file_dialog(std::string const& filter = "All Files\0*.*\0") {
	OPENFILENAME ofn;       // common dialog box structure
	wchar_t file[260] = {}; // buffer for file name
	HWND hwnd{};            // owner window
	HANDLE hf{};            // file handle

	wchar_t wfilter[256] = {};
	strtowstr(filter, wfilter);

	// initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = file;
	// set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of file to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(file);
	ofn.lpstrFilter = wfilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileName(&ofn) == TRUE) {
		return wstrtostr(ofn.lpstrFile);
	}

	return std::string(); // empty if canceled
}

EditorGUI::EditorGUI() {
	m_viewport_framebuffer = Framebuffer::create(g_viewport_resolutions[m_current_resolution][0], g_viewport_resolutions[m_current_resolution][1],
		{
			{ ImageFormat::RGBA8 },
			{ ImageFormat::DepthStencil },
		});
}

EditorGUI::~EditorGUI() {

}

void EditorGUI::draw() {
	PROFILE_FUNCTION();
	if (m_dockspace) {
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;
		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
		ImGuiViewport const* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		if (ImGui::Begin("dockspace", nullptr, window_flags)) {
			ImGui::PopStyleVar(3);

			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
				ImGuiID dockspace_id = ImGui::GetID("dockspace");
				ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
			}

			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("import")) {
					if (ImGui::MenuItem("wavefront (.obj)")) {
						Filepath file = open_file_dialog("Wavefront OBJ\0*.obj\0All Files\0*.*\0");
						std::cout << "import file: " << file.generic_string() << "\n";
						Filepath cwd = std::filesystem::current_path();
						std::cout << "current working directory: " << cwd.generic_string() << std::endl;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}
		}

		ImGui::End();
	}

	if (m_window_viewport) {
		if (ImGui::Begin("viewport", nullptr, ImGuiWindowFlags_MenuBar)) {
			m_is_viewport_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

			if (m_draw_viewport_overlay_func) {
				m_draw_viewport_overlay_func();
			}

			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("resolution")) {
					for (int i = 0; i < ARRAY_LENGTH(g_viewport_resolutions); ++i) {
						if (ImGui::MenuItem(
							(std::to_string(g_viewport_resolutions[i][0]) + "x" + std::to_string(g_viewport_resolutions[i][1])).c_str(),
							nullptr, nullptr, i != m_current_resolution)) {
							m_current_resolution = i;
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			auto w = (float)m_viewport_framebuffer->get_description().width;
			auto h = (float)m_viewport_framebuffer->get_description().height;

			if (w != g_viewport_resolutions[m_current_resolution][0] || h != g_viewport_resolutions[m_current_resolution][1]) {
				m_viewport_framebuffer->resize(g_viewport_resolutions[m_current_resolution][0], g_viewport_resolutions[m_current_resolution][1]);
			}

			ImVec2 present_size;
			auto region = ImGui::GetContentRegionAvail();
			if (region.y / region.x > h / w) {
				present_size.x = region.x;
				present_size.y = region.x * h / w;
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (region.y - present_size.y) / 2);
			}
			else {
				present_size.x = region.y * w / h;
				present_size.y = region.y;
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (region.x - present_size.x) / 2);
			}
			m_viewport_pixel_scale_x = 1.0f / present_size.x;

			ImGui::BeginChild("viewport_child", present_size, false, ImGuiWindowFlags_NoScrollbar);
			ImGui::Image(m_viewport_framebuffer->get_attachment_native_handle(0), present_size, ImVec2(0, 1), ImVec2(1, 0));
			m_is_viewport_hovered = ImGui::IsItemHovered();

			if (m_is_viewport_hovered) {
				ImVec2 image_min = ImGui::GetItemRectMin(); // top-left corner of the last item
				ImVec2 image_max = ImGui::GetItemRectMax(); // bottom-right corner (optional)
				ImVec2 mouse_pos = ImGui::GetMousePos();

				// relative mouse position inside the image
				ImVec2 local_pos = ImVec2(mouse_pos.x - image_min.x, mouse_pos.y - image_min.y);
				ImVec2 image_rect = ImVec2(image_max.x - image_min.x, image_max.y - image_min.y);

				m_viewport_mouse_x = local_pos.x / image_rect.x;
				m_viewport_mouse_y = 1.0f - local_pos.y / image_rect.y;
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	static bool show = true;
	ImGui::ShowDemoWindow(&show);

	if (ImGui::Begin("text window")) {
		ImGui::Text("hello world!");
	}
	ImGui::End();
}
