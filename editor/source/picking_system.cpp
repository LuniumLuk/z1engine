#include "picking_system.h"

PickingSystem::PickingSystem(uint32_t w, uint32_t h) {
	m_framebuffer = Framebuffer::create(w, h,
		{
			{ ImageFormat::RGBA8 },
			{ ImageFormat::Depth },
		});

	{
		Pipeline::Description desc{};
		desc.depth_test = true;
		desc.blend = false;
		desc.cull_mode = CullMode::Back;
		desc.shader = g_runtime_context.m_asset_manager->get<Shader>("picking");
		m_pipeline = Pipeline::build(desc);
	}

	{
		Pipeline::Description desc{};
		desc.depth_test = true;
		desc.blend = false;
		desc.cull_mode = CullMode::Back;
		desc.shader = g_runtime_context.m_asset_manager->get<Shader>("picking_sprite");
		m_sprite_pipeline = Pipeline::build(desc);
	}

	m_render_pass = RenderPass::build();

	std::vector<glm::vec3> quad_vertices = {
		{-0.5f, -0.5f, 0.0f},
		{ 0.5f, -0.5f, 0.0f},
		{ 0.5f,  0.5f, 0.0f},
		{-0.5f, -0.5f, 0.0f},
		{ 0.5f,  0.5f, 0.0f},
		{-0.5f,  0.5f, 0.0f},
	};

	auto vertex_buffer = VertexBuffer::create(quad_vertices.data(), quad_vertices.size() * sizeof(glm::vec3),
		{
			{ DataType::Float3 },
		});
	m_quad_vao = VertexArray::create({ vertex_buffer });
}

void PickingSystem::render(std::shared_ptr<Scene> const& scene) const {
	auto const& main_cam = scene->get_main_camera();
	auto& camera_comp = main_cam->get_component<CameraComponent>();

	auto& camera_trans = main_cam->get_component<TransformComponent>();

	auto cam_up = camera_trans.get_world_transform() * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
	auto cam_forward = camera_trans.get_world_transform() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
	auto cam_view = glm::lookAt(camera_trans.m_location, camera_trans.m_location + glm::vec3(cam_forward), glm::vec3(cam_up));

	glm::mat4 cam_projview = camera_comp.get_proj() * cam_view;

	RenderPass::BeginInfo info{};
	info.framebuffer = m_framebuffer;
	info.clear_color = true;
	info.clear_depth = true;
	info.clear_color_value = { 0.0f, 0.0f, 0.0f, 0.0f };
	info.clear_depth_value = 1.0f;
	m_render_pass->bind(info);

	m_pipeline->bind();
	m_pipeline->m_shader->set_uniform("u_projview", &cam_projview);
	{
		auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const, TagComponent const>();
		for (auto [entity, transform, mesh, tag] : view.each()) {
			float object_id = static_cast<float>(tag.m_id) + 1.0f;
			m_pipeline->m_shader->set_uniform("u_model", &transform.get_world_transform());
			m_pipeline->m_shader->set_uniform("u_object_id", &object_id);
			mesh.m_mesh->draw();
		}
	}
	m_pipeline->unbind();

	m_sprite_pipeline->bind();
	m_sprite_pipeline->m_shader->set_uniform("u_projview", &cam_projview);
	m_quad_vao->bind();
	{
		auto view = scene->m_registry.view<TransformComponent const, SpriteComponent const, TagComponent const>();
		for (auto [entity, transform, sprite, tag] : view.each()) {
			float object_id = static_cast<float>(tag.m_id) + 1.0f;
			m_sprite_pipeline->m_shader->set_uniform("u_model", &transform.get_world_transform());
			m_sprite_pipeline->m_shader->set_uniform("u_object_id", &object_id);
			m_quad_vao->draw(PrimitiveType::Triangles);
		}
	}
	m_quad_vao->unbind();
	m_sprite_pipeline->unbind();

	m_render_pass->unbind();
}

uint32_t PickingSystem::unpack_rgba8_to_uint32(glm::u8vec4 const& rgba) const {
	return
		((uint32_t)rgba.r << 0) |
		((uint32_t)rgba.g << 8) |
		((uint32_t)rgba.b << 16) |
		((uint32_t)rgba.a << 24);
}

uint32_t PickingSystem::query(float x, float y) const {
	glm::u8vec4 rgba{};
	m_framebuffer->read_pixel(0,
		static_cast<uint32_t>(x * Framebuffer::get_width(m_framebuffer)),
		static_cast<uint32_t>(y * Framebuffer::get_height(m_framebuffer)),
		&rgba[0]);
	auto object_id = unpack_rgba8_to_uint32(rgba);
	if (object_id) {
		return object_id - 1;
	}
	else {
		return INVALID_INDEX;
	}
}
