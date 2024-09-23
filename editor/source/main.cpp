#include <iostream>
#include <z1engine.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui_ui.h>

using namespace z1;

struct MyLayer : Layer {
    MyLayer(std::shared_ptr<ImGuiUILayer> imgui_ui_layer): m_imgui_ui_layer(imgui_ui_layer), m_framebuffer(m_imgui_ui_layer->get_viewport_framebuffer()) {

    }

    void on_attach() override {
        m_camera = Camera::create_ortho(0.0f, 0.0f, 1.0f, 1.0f);
        m_camera_controller = std::make_shared<Generic2DCameraController>(m_camera);
        g_runtime_context.m_main_camera = m_camera;
        g_runtime_context.m_main_framebuffer = m_framebuffer;
        m_texture = Image2D::create(g_runtime_context.m_file_system->m_engine_dir / "asset/texture/awesomeface.png");
    }

    void on_update(double delta_time) override {
        m_fps_timer += delta_time;
        m_fps_counter += 1;
        if (m_fps_timer >= 1.0) {
            g_runtime_context.m_window->set_window_title(std::string("z1 engine fps: ") + std::to_string(m_fps_counter));
            m_fps_timer = 0.0;
            m_fps_counter = 0;
        }
        m_camera_controller->m_drag_speed = m_imgui_ui_layer->m_viewport_pixel_scale_x;

        m_input_state.update(delta_time);
        if (m_imgui_ui_layer->is_viewport_focused()) {
            m_camera_controller->update(m_input_state);
        }
        m_input_state.reset();

        g_runtime_context.m_renderer_2d->draw_quad({ 0.2f, 0.4f, 0.1f }, { 0.2f, 0.4f }, { 1.0f, 0.0f, 1.0f, 1.0f });
        g_runtime_context.m_renderer_2d->draw_quad({ 0.4f, 0.2f, 0.2f }, { 0.4f, 0.1f }, { 0.0f, 1.0f, 1.0f, 1.0f });
        g_runtime_context.m_renderer_2d->draw_quad({ 0.0f, 0.0f, 0.0f }, { 0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f, 1.0f }, m_texture);

        g_runtime_context.m_renderer_2d->prepare_draw();
        g_runtime_context.m_renderer_2d->draw();
    }

    void on_event(Event& event) override {
        m_input_state.on_event(event);
    }

    void on_imgui_render() override {
        ImGui::Begin("resource info");
        ImGui::Text("resources");
        if (ImGui::BeginTable("resources", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("id");
            ImGui::TableSetupColumn("type");
            ImGui::TableSetupColumn("binding");
            ImGui::TableSetupColumn("ref count");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            for (auto const& resource : g_runtime_context.m_resource_manager->m_resources) {
                if (!resource) continue;
                ImGui::TableNextColumn(); ImGui::Text(std::to_string(resource->get_resource_id()).c_str());
                ImGui::TableNextColumn(); ImGui::Text(get_resource_name(resource->get_resource_type()).c_str());
                ImGui::TableNextColumn(); ImGui::Text(resource->get_binding() == INVALID_BINDING ? "none" : std::to_string(resource->get_binding()).c_str());
                ImGui::TableNextColumn(); ImGui::Text(std::to_string(resource->get_ref_count()).c_str());
            }
            ImGui::EndTable();
        }
        ImGui::End();

        ImGui::Begin("debug");
        if (ImGui::RadioButton("v sync", g_runtime_context.m_window->is_v_sync_enabled())) {
            g_runtime_context.m_window->set_v_sync(!g_runtime_context.m_window->is_v_sync_enabled());
        }
        ImGui::Text(std::string("viewport_pixel_scale_x: " + std::to_string(m_imgui_ui_layer->m_viewport_pixel_scale_x)).c_str());
        ImGui::End();
    }

private:
    std::shared_ptr<ImGuiUILayer> m_imgui_ui_layer;
    std::shared_ptr<Framebuffer> m_framebuffer;
    std::shared_ptr<Camera> m_camera;
    std::shared_ptr<Generic2DCameraController> m_camera_controller;
    InputState m_input_state;
    std::shared_ptr<Image2D> m_texture;

    int m_fps_counter = 0;
    double m_fps_timer = 0.0;

    void show_shader_info(std::shared_ptr<Shader> const& shader) {
        ImGui::Begin("shader info");
        ImGui::Text("basic info");
        if (ImGui::BeginTable("basic info", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
            ImGui::TableNextColumn(); ImGui::Text("name"); ImGui::TableNextColumn(); ImGui::Text(shader->get_name().c_str());
            ImGui::TableNextColumn(); ImGui::Text("path"); ImGui::TableNextColumn(); ImGui::Text(shader->get_path().c_str());
            ImGui::EndTable();
        }
        ImGui::Text("shader attributes");
        if (ImGui::BeginTable("shader attributes", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("name");
            ImGui::TableSetupColumn("type");
            ImGui::TableSetupColumn("location");
            ImGui::TableSetupColumn("count");
            ImGui::TableHeadersRow();

            for (auto& attrib : shader->get_attributes()) {
                ImGui::TableNextColumn(); ImGui::Text(attrib.m_name.c_str());
                ImGui::TableNextColumn(); ImGui::Text((get_data_type_name(attrib.m_type)).c_str());
                ImGui::TableNextColumn(); ImGui::Text(std::to_string(attrib.m_location).c_str());
                ImGui::TableNextColumn(); ImGui::Text(std::to_string(attrib.m_count).c_str());
            }
            ImGui::EndTable();
        }
        ImGui::Text("shader uniforms");
        if (ImGui::BeginTable("shader uniforms", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("name");
            ImGui::TableSetupColumn("type");
            ImGui::TableSetupColumn("location");
            ImGui::TableSetupColumn("count");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            for (auto& uniform : shader->get_uniforms()) {
                if (uniform.m_location == INVALID_LOCATION) continue;
                ImGui::TableNextColumn(); ImGui::Text(uniform.m_name.c_str());
                ImGui::TableNextColumn(); ImGui::Text((get_data_type_name(uniform.m_type)).c_str());
                ImGui::TableNextColumn(); ImGui::Text(std::to_string(uniform.m_location).c_str());
                ImGui::TableNextColumn(); ImGui::Text(std::to_string(uniform.m_count).c_str());
            }
            ImGui::EndTable();
        }
        ImGui::Text("shader uniform blocks");
        for (auto& block : shader->get_uniform_blocks()) {
            if (ImGui::BeginTable("shader uniform blocks", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
                ImGui::TableNextColumn(); ImGui::Text("name"); ImGui::TableNextColumn(); ImGui::Text(block.m_name.c_str());
                ImGui::TableNextColumn(); ImGui::Text("size"); ImGui::TableNextColumn(); ImGui::Text(std::to_string(block.m_size).c_str());
                ImGui::TableNextColumn(); ImGui::Text("binding"); ImGui::TableNextColumn(); ImGui::Text(std::to_string(block.m_binding).c_str());

                ImGui::TableNextColumn(); ImGui::Text("variables"); ImGui::TableNextColumn();
                if (ImGui::BeginTable("variables", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
                    ImGui::TableSetupColumn("name");
                    ImGui::TableSetupColumn("type");
                    ImGui::TableSetupColumn("count");
                    ImGui::TableHeadersRow();

                    for (auto& variable : block.m_variables) {
                        ImGui::TableNextColumn(); ImGui::Text(variable.m_name.c_str());
                        ImGui::TableNextColumn(); ImGui::Text((get_data_type_name(variable.m_type)).c_str());
                        ImGui::TableNextColumn(); ImGui::Text(std::to_string(variable.m_count).c_str());
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();
    }
};

struct MyApp : Application {
    void init() override {
        m_imgui_ui_layer = std::make_shared<ImGuiUILayer>();

        push_layer(std::make_shared<MyLayer>(m_imgui_ui_layer));
        push_overlay(m_imgui_ui_layer);
    };

private:
    std::shared_ptr<ImGuiUILayer> m_imgui_ui_layer;
};

int main() {
    std::cout << "hello world!\n";

    MyApp app;

    app.init();

    app.run();

    return 0;
}
