#include <iostream>
#include <z1engine.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <gui.h>

using namespace z1;

struct EditorLayer : Layer {
	EditorLayer() {
		m_gui = std::make_shared<EditorGUI>();
		m_active_scene = std::make_shared<Scene>();

		m_texture = io::load_image2d(g_runtime_context.m_file_system->m_engine_dir / "asset/texture/awesomeface.png");
		m_checker = io::load_image2d(g_runtime_context.m_file_system->m_engine_dir / "asset/texture/tira-checker.jpg");
		m_atlas = io::load_image2d("asset/texture/roguelikeSheet_transparent.png", SamplerMode::Nearest);

		{
			auto ent = m_active_scene->create_entity("Square_0");
			ent->add_component<SpriteComponent>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), m_texture);
			m_entities.push_back(ent);
		}

		{
			auto ent = m_active_scene->create_entity("Square_1");
			ent->add_component<SpriteComponent>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), m_checker);
			ent->get_component<TransformComponent>().m_location = glm::vec3(0.0f, 0.0f, -9.5f);
			ent->get_component<TransformComponent>().m_scale = glm::vec3(10.0f, 10.0f, 1.0f);
			m_entities.push_back(ent);
		}

		m_camera = Camera::create_ortho(0.0f, 0.0f, 1.0f, 1.0f, -10.0f, 10.0f);
		m_camera_controller = std::make_shared<Generic2DCameraController>(m_camera);
		g_runtime_context.m_main_camera = m_camera;
		g_runtime_context.m_main_framebuffer = m_gui->get_viewport_framebuffer();

		{
			auto ent = m_active_scene->create_entity("Mesh_0");
			auto mesh = io::load_static_mesh("asset/mesh/bunny.obj");
			ent->add_component<StaticMeshComponent>(mesh);
			ent->get_component<TransformComponent>().m_location = glm::vec3(0.0f, 0.0f, -5.0f);
			m_entities.push_back(ent);
		}

		uint32_t tile_size = 16;
		uint32_t stride = 1;
		for (uint32_t i = 0; i < 4; ++i) { // 50
			for (uint32_t j = 0; j < 3; ++j) { // 30
				auto tile = SubImage2D::create(m_atlas, i * (tile_size + stride), j * (tile_size + stride), tile_size, tile_size, true);
				auto ent = m_active_scene->create_entity("SubImage2D_" + std::to_string(i * 50 + j));
				ent->add_component<SpriteComponent>(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), tile);
				ent->get_component<TransformComponent>().m_location = glm::vec3(-(float)i * m_quad_stride - 0.2f, -(float)j * m_quad_stride - 0.2f, 1.0f);
				ent->get_component<TransformComponent>().m_scale = glm::vec3(m_quad_size, m_quad_size, 1.0f);
				m_entities.push_back(ent);
			}
		}

		for (int i = 0; i < m_quad_rows; ++i) {
			for (int j = 0; j < m_quad_cols; ++j) {
				auto ent = m_active_scene->create_entity("Quad_" + std::to_string(i * 50 + j));
				ent->add_component<SpriteComponent>(glm::vec4((float)(i % m_quad_rows) / m_quad_rows, (float)(j % m_quad_cols) / m_quad_cols, 1.0f, 1.0f));
				ent->get_component<TransformComponent>().m_location = glm::vec3(i * m_quad_stride, j * m_quad_stride, 0.1f);
				ent->get_component<TransformComponent>().m_scale = glm::vec3(m_quad_size, m_quad_size, 1.0f);
				m_entities.push_back(ent);
			}
		}
	}

	void on_attach() override {

	}

	void on_update(float delta_time) override {
		m_fps_timer += delta_time;
		m_fps_counter += 1;
		if (m_fps_timer >= 1.0) {
			g_runtime_context.m_window->set_window_title(std::string("z1 engine fps: ") + std::to_string(m_fps_counter));
			m_fps_timer = 0.0;
			m_fps_counter = 0;
		}
		m_camera_controller->m_drag_speed = m_gui->m_viewport_pixel_scale_x;

		m_input_state.update(delta_time);
		if (m_gui->is_viewport_focused()) {
			m_camera_controller->update(m_input_state);
		}
		m_input_state.reset();

		m_active_scene->on_update(delta_time);
	}

	void on_event(Event& event) override {
		m_input_state.on_event(event);
	}

	void on_imgui_render() override {
		m_gui->draw();

		ImGui::Begin("resource info");
		ImGui::Text("resources");
		if (ImGui::BeginTable("resources", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
			ImGui::TableSetupColumn("id");
			ImGui::TableSetupColumn("type");
			ImGui::TableSetupColumn("binding");
			ImGui::TableSetupColumn("ref count");
			ImGui::TableSetupColumn("info");
			ImGui::TableHeadersRow();

			ImGui::TableNextRow();
			for (auto const& resource : g_runtime_context.m_resource_manager->m_resources) {
				if (!resource) continue;
				ImGui::TableNextColumn(); ImGui::Text(std::to_string(resource->get_resource_id()).c_str());
				ImGui::TableNextColumn(); ImGui::Text(get_resource_name(resource->get_resource_type()).c_str());
				ImGui::TableNextColumn(); ImGui::Text(resource->get_binding() == INVALID_BINDING ? "none" : std::to_string(resource->get_binding()).c_str());
				ImGui::TableNextColumn(); ImGui::Text(std::to_string(resource->get_ref_count()).c_str());
				switch (resource->get_resource_type()) {
				case ResourceType::Image:
					ImGui::TableNextColumn(); ImGui::Text(get_image_info(g_runtime_context.m_resource_manager->get<Image>(resource->get_resource_id())).c_str()); break;
				case ResourceType::UniformBuffer:
					ImGui::TableNextColumn(); ImGui::Text(get_uniform_buffer_info(g_runtime_context.m_resource_manager->get<UniformBuffer>(resource->get_resource_id())).c_str()); break;
				}
			}
			ImGui::EndTable();
		}
		ImGui::End();

		ImGui::Begin("debug");
		if (ImGui::RadioButton("v sync", g_runtime_context.m_window->is_v_sync_enabled())) {
			g_runtime_context.m_window->set_v_sync(!g_runtime_context.m_window->is_v_sync_enabled());
		}
		ImGui::InputInt("quad rows", &m_quad_rows);
		ImGui::InputInt("quad cols", &m_quad_cols);
		ImGui::InputFloat("quad stride", &m_quad_stride);
		ImGui::InputFloat("quad size", &m_quad_size);
		ImGui::Text(std::string("viewport_pixel_scale_x: " + std::to_string(m_gui->m_viewport_pixel_scale_x)).c_str());
		ImGui::End();
	}

private:
	std::shared_ptr<EditorGUI> m_gui;
	std::shared_ptr<Scene> m_active_scene;
	std::vector<std::shared_ptr<Entity>> m_entities;
	std::shared_ptr<Camera> m_camera;
	std::shared_ptr<Generic2DCameraController> m_camera_controller;
	InputState m_input_state;
	std::shared_ptr<Image2D> m_texture;
	std::shared_ptr<Image2D> m_checker;
	std::shared_ptr<Image2D> m_atlas;

	int m_quad_rows = 4;
	int m_quad_cols = 2;
	float m_quad_stride = 0.15f;
	float m_quad_size = 0.1f;

	int m_fps_counter = 0;
	float m_fps_timer = 0.0;

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

	std::string get_image_info(Image* image) {
		auto const& desc = image->get_description();
		std::string info = "";

		info += std::to_string(desc.m_width) + "x" + std::to_string(desc.m_height) + " ";
		info += get_image_format_name(desc.m_format) + " ";
		info += get_sampler_mode_name(desc.m_sampler_mode) + " ";
		info += get_wrap_mode_name(desc.m_wrap_mode);

		return info;
	}

	std::string get_uniform_buffer_info(UniformBuffer* buffer) {
		std::string info = "size: ";

		info += std::to_string(buffer->get_size()) + " byte";

		return info;
	}
};

struct EditorApp : Application {
	void init() override {
		push_layer(std::make_shared<EditorLayer>());
	};
};

#include <chrono>

int main() {
	std::cout << "hello world!\n";

	EditorApp app;

	app.init();

	app.run();

	return 0;
}
