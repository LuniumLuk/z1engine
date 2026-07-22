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
		desc.depth_write = true;
		desc.blend = false;
		desc.cull_mode = CullMode::Back;
		desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/picking");
		m_pipeline = Pipeline::build(desc);
	}

	{
		Pipeline::Description desc{};
		desc.depth_test = true;
		desc.depth_write = true;
		desc.blend = false;
		desc.cull_mode = CullMode::Back;
		desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/picking_sprite");
		m_sprite_pipeline = Pipeline::build(desc);
	}

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

	RenderPass::Description desc{};
	desc.color_attachments.resize(1);
	desc.color_attachments[0].load_op = LoadOp::Clear;
	desc.color_attachments[0].clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };
	desc.depth_stencil_attachment.depth_load_op = LoadOp::Clear;
	desc.depth_stencil_attachment.clear_depth_value = 1.0f;
	auto render_pass = std::make_shared<RenderPass>(desc);

	render_pass->execute = [this, scene](GraphicsContext& ctx) {
		auto const& main_cam = scene->get_main_camera();
		if (!main_cam) {
			return;
		}

		auto& camera_comp = main_cam->get_component<CameraComponent>();
		glm::mat4 cam_projview = camera_comp.get_proj() * camera_comp.get_view();

		m_pipeline->bind();
		g_runtime_context.m_global->bind();
		m_pipeline->m_shader->set_uniform_block_binding("Global", g_runtime_context.m_global->get_binding());

		auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const, TagComponent const>();
		for (auto [entity, transform, mesh, tag] : view.each()) {
			if (!mesh.m_mesh)
				continue;

			float object_id = static_cast<float>(tag.m_id) + 1.0f;

			auto model = transform.get_world_transform();
			m_pipeline->m_shader->set_uniform("u_model", &model);
			m_pipeline->m_shader->set_uniform("u_object_id", &object_id);

			mesh.m_mesh->draw();
		}
		auto view_skel =
			scene->m_registry.view<TransformComponent const, SkeletalMeshComponent const, TagComponent const>();
		for (auto [entity, transform, mesh, tag] : view_skel.each()) {
			if (!mesh.m_mesh)
				continue;

			float object_id = static_cast<float>(tag.m_id) + 1.0f;

			auto model = transform.get_world_transform();
			m_pipeline->m_shader->set_uniform("u_model", &model);
			m_pipeline->m_shader->set_uniform("u_object_id", &object_id);

			std::shared_ptr<UniformBuffer> bones = nullptr;
			if (scene->m_registry.all_of<AnimationComponent>(entity)) {
				auto const& anim = scene->m_registry.get<AnimationComponent>(entity);
				bones = anim.bone_ubo;
			}
			int has_skinning = 0;
			if (bones) {
				has_skinning = 1;
				bones->bind();
				m_pipeline->m_shader->set_uniform_block_binding("Bones", bones->get_binding());
			}
			m_pipeline->m_shader->set_uniform("u_has_skinning", &has_skinning);
			mesh.m_mesh->draw();
			if (bones) {
				bones->unbind();
			}
		}

		g_runtime_context.m_global->unbind();

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
		};

	g_runtime_context.m_graphics_context->bind_framebuffer(m_framebuffer);
	g_runtime_context.m_graphics_context->exec_render_pass(render_pass);
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
		static_cast<uint32_t>(x * m_framebuffer->get_width()),
		static_cast<uint32_t>(y * m_framebuffer->get_height()),
		&rgba[0]);
	auto object_id = unpack_rgba8_to_uint32(rgba);
	if (object_id) {
		return object_id - 1;
	}
	else {
		return INVALID_INDEX;
	}
}
