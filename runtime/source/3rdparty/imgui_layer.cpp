#include "pch.h"
#include "3rdparty/imgui_layer.h"
#include "core/core.h"
#include "core/window.h"
#include "core/application.h"
#include "imgui.h"
#include "glad/glad.h"
#include "glfw/glfw3.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imguizmo/ImGuizmo.h"

namespace z1 {

	ImGuiLayer::ImGuiLayer() : Layer("imgui layer") {

	}

	ImGuiLayer::~ImGuiLayer() {
		CORE_DEBUG("shutting down ImGuiLayer ...");
	}

	void ImGuiLayer::on_attach() {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// TODO setup window according to the platform
		auto window = static_cast<GLFWwindow*>(g_runtime_context.m_window->get_native_window());

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 460");
	}

	void ImGuiLayer::on_detach() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::begin() {
		PROFILE_FUNCTION();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiLayer::end() {
		PROFILE_FUNCTION();
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)g_runtime_context.m_window->get_width(), (float)g_runtime_context.m_window->get_height());

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* currentContext = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(currentContext);
		}
	}

}
