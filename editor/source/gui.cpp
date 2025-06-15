#include "gui.h"

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
		ImGui::Begin("dockspace", nullptr, window_flags);
		ImGui::PopStyleVar(3);

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
			ImGuiID dockspace_id = ImGui::GetID("dockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("options")) {
				if (ImGui::MenuItem("close"))
					m_dockspace = false;
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::End();
	}

	if (m_window_viewport) {
		ImGui::Begin("viewport", nullptr, ImGuiWindowFlags_MenuBar);

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

		auto w = (float)m_viewport_framebuffer->get_description().m_width;
		auto h = (float)m_viewport_framebuffer->get_description().m_height;

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
		m_is_viewport_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
		ImGui::Image(m_viewport_framebuffer->get_attachment_native_handle(0), present_size, ImVec2(0, 1), ImVec2(1, 0));
		ImGui::EndChild();

		ImGui::End();
	}

	static bool show = true;
	ImGui::ShowDemoWindow(&show);

	ImGui::Begin("text window");
	ImGui::Text("hello world!");
	ImGui::End();
}
