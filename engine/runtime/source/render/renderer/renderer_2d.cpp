#include "pch.h"
#include "render/shader.h"
#include "render/framebuffer.h"
#include "render/render_graph.h"
#include "render/graphics_context.h"
#include "render/renderer/renderer_2d.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "scene/component/camera.h"
#include "scene/component/sprite.h"
#include "asset/asset_manager.h"
#include "glm/gtc/matrix_transform.hpp"

namespace z1 {

	Renderer2D::Renderer2D() {
		m_vertex_buffer = VertexBuffer::create(nullptr, s_max_quad_vertices_per_batch * sizeof(QuadVertex),
			{
				{DataType::Float3},
				{DataType::Float2},
				{DataType::Float4},
				{DataType::Float},
				{DataType::Float4},
			}, BufferUsage::Stream);

		auto vertex_indices = new uint32_t[s_max_quad_indices_per_batch];
		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_max_quad_indices_per_batch; i += 6) {
			vertex_indices[i + 0] = offset + 0;
			vertex_indices[i + 1] = offset + 1;
			vertex_indices[i + 2] = offset + 2;

			vertex_indices[i + 3] = offset + 0;
			vertex_indices[i + 4] = offset + 2;
			vertex_indices[i + 5] = offset + 3;

			offset += 4;
		}
		auto index_buffer = IndexBuffer::create(vertex_indices, s_max_quad_indices_per_batch * sizeof(uint32_t), BufferUsage::Static);

		m_vertex_array = VertexArray::create({ m_vertex_buffer }, index_buffer);

		Pipeline::Description desc{};
		desc.depth_test = true;
		desc.blend = true;
		desc.shader = g_runtime_context.m_asset_manager->get<Shader>("$engine/shader/sprite_2d_batched");
		m_pipeline = Pipeline::build(desc);

		uint32_t white = 0xffffffff;
		m_default_image = Image2D::create(&white, sizeof(white), 1, 1, ImageFormat::RGBA8);
	}

	Renderer2D::~Renderer2D() {

	}

	void Renderer2D::draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer) {
		PROFILE_FUNCTION();

		std::vector<Renderer2D::Quad> quads;
		if (scene) {
			auto view = scene->m_registry.view<TransformComponent const, SpriteComponent const>();
			for (auto [entity, transform, sprite] : view.each()) {
				Renderer2D::Quad quad{};
				quad.m_transform = transform.get_world_transform();
				quad.m_color = sprite.m_color;
				quad.m_texture = sprite.m_texture ? sprite.m_texture->m_image : nullptr;
				quad.m_tiling_scale = sprite.m_tiling_scale;
				quad.m_tiling_offset = sprite.m_tiling_offset;
				quad.m_texcoords = sprite.m_texcoords;
				quads.push_back(quad);
			}
		}

		draw_quads(quads);
		prepare_draw(framebuffer);

		auto rg = RenderGraph();

		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::Load;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Load;

		rg.add_pass("main")
			.set_output(framebuffer)
			.set_pass_desc(desc)
			.execute([&](RenderGraphNode& node, GraphicsContext& ctx) {
				auto const& cam = scene->get_main_camera();

				auto& camera_comp = cam->get_component<CameraComponent>();
				if (!camera_comp.m_use_fixed_aspect) {
					camera_comp.m_aspect = node.get_aspect();
				}

				auto projview = camera_comp.get_proj() * camera_comp.get_view();

				m_pipeline->bind();
				m_pipeline->m_shader->set_uniform("u_projview", &projview);
				batch_draw();
				m_pipeline->unbind();
				});

		rg.compile();
		rg.execute();

		after_draw();
	}

	void Renderer2D::prepare_draw(std::shared_ptr<Framebuffer> const& framebuffer) {
		PROFILE_FUNCTION();

		if (m_quads.empty()) return;

		std::sort(m_quads.begin(), m_quads.end(), [](QuadData const& a, QuadData const& b) {
			if (a.m_model[3][2] == b.m_model[3][2]) {
				return a.m_image < b.m_image;
			}
			else {
				return a.m_model[3][2] < b.m_model[3][2];
			}
		});

		std::vector<glm::vec4> quad_pos = {
			{-0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f, -0.5f, 0.0f, 1.0f},
			{ 0.5f,  0.5f, 0.0f, 1.0f},
			{-0.5f,  0.5f, 0.0f, 1.0f},
		};

		m_quad_vertices.resize(m_quads.size() * 4);
		m_batches.resize(1);
		m_batches[0].m_index_offset = 0;
		m_batches[0].m_vertex_offset = 0;
		uint32_t curr_batch = 0;
		uint32_t vertex_offset = 0;
		uint32_t index_offset = 0;
		for (auto const& quad : m_quads) {
			float texture_id = 0;

			if ((vertex_offset - m_batches[curr_batch].m_vertex_offset) == s_max_quad_vertices_per_batch) {
				m_batches[curr_batch].m_index_num = index_offset - m_batches[curr_batch].m_index_offset;
				m_batches[curr_batch].m_vertex_num = vertex_offset - m_batches[curr_batch].m_vertex_offset;
				m_batches.push_back({});
				curr_batch += 1;
				m_batches[curr_batch].m_index_offset = index_offset;
				m_batches[curr_batch].m_vertex_offset = vertex_offset;
			}

			if (m_batches[curr_batch].m_textures.find(quad.m_image) != m_batches[curr_batch].m_textures.end()) {
				texture_id = (float)m_batches[curr_batch].m_textures[quad.m_image];
			}
			else {
				if (m_batches[curr_batch].m_textures.size() == 32) {
					m_batches[curr_batch].m_index_num = index_offset - m_batches[curr_batch].m_index_offset;
					m_batches[curr_batch].m_vertex_num = vertex_offset - m_batches[curr_batch].m_vertex_offset;
					m_batches.push_back({});
					curr_batch += 1;
					m_batches[curr_batch].m_index_offset = index_offset;
					m_batches[curr_batch].m_vertex_offset = vertex_offset;
				}

				texture_id = (float)m_batches[curr_batch].m_textures.size();
				m_batches[curr_batch].m_textures[quad.m_image] = (uint32_t)texture_id;
			}

			for (int i = 0; i < 4; ++i) {
				auto pos = quad.m_model * quad_pos[i];
				m_quad_vertices[vertex_offset + i].m_position = quad.m_model * quad_pos[i];
				m_quad_vertices[vertex_offset + i].m_texcoord = quad.m_texcoords[i];
				m_quad_vertices[vertex_offset + i].m_color = quad.m_color;
				m_quad_vertices[vertex_offset + i].m_texture_id = texture_id;
				m_quad_vertices[vertex_offset + i].m_tiling_factor = quad.m_tiling_factor;
			}

			vertex_offset += 4;
			index_offset += 6;
		}
		m_batches[curr_batch].m_index_num = index_offset - m_batches[curr_batch].m_index_offset;
		m_batches[curr_batch].m_vertex_num = vertex_offset - m_batches[curr_batch].m_vertex_offset;
	}

	void Renderer2D::batch_draw() {
		PROFILE_FUNCTION();
		for (auto const& batch : m_batches) {
			m_vertex_buffer->write(&m_quad_vertices[batch.m_vertex_offset], batch.m_vertex_num * sizeof(QuadVertex));

			std::vector<int> bindings(32);
			for (auto const& [texture, index] : batch.m_textures) {
				if (!texture->is_bound()) {
					texture->bind();
				}
				bindings[index] = texture->get_binding();
			}
			m_pipeline->m_shader->set_uniform("u_texture[0]", bindings.data());

			m_vertex_array->bind();
			m_vertex_array->draw(PrimitiveType::Triangles, batch.m_index_num);
			m_vertex_array->unbind();

			for (auto const& [texture, index] : batch.m_textures) {
				if (texture->is_bound()) {
					texture->unbind();
				}
			}
		}
	}

	void Renderer2D::after_draw() {
		PROFILE_FUNCTION();
		m_quads.clear();
	}

	void Renderer2D::draw_quad(
		glm::vec3 const& position,
		glm::vec2 const& size,
		float rotation,
		glm::vec4 const& color) {
		glm::mat4 model = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		m_quads.push_back({ model, color, m_default_image });
	}

	void Renderer2D::draw_quad(
		glm::vec3 const& position,
		glm::vec2 const& size,
		float rotation,
		glm::vec4 const& color,
		std::shared_ptr<Texture2D> const& texture,
		glm::vec2 const& tiling_scale,
		glm::vec2 const& tiling_offset) {
		glm::mat4 model = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		m_quads.push_back({ model, color, texture ? texture->m_image : m_default_image, { tiling_scale.x, tiling_scale.y, tiling_offset.x, tiling_offset.y } });
	}

	void Renderer2D::draw_quad(
		glm::vec3 const& position,
		glm::vec2 const& size,
		float rotation,
		glm::vec4 const& color,
		std::shared_ptr<SubTexture2D> const& texture) {

		glm::mat4 model = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		QuadData quad = {};
		quad.m_model = model;
		quad.m_color = color;
		if (texture) {
			quad.m_image = texture->m_texture->m_image;
			quad.m_texcoords = texture->m_texcoords;
		}
		else {
			quad.m_image = m_default_image;
		}

		m_quads.push_back(quad);
	}

	void Renderer2D::draw_quads(std::vector<Quad> const& quads) {
		for (auto const& quad : quads) {
			m_quads.push_back({ quad.m_transform, quad.m_color, quad.m_texture ? quad.m_texture : m_default_image, { quad.m_tiling_scale.x, quad.m_tiling_scale.y, quad.m_tiling_offset.x, quad.m_tiling_offset.y }, quad.m_texcoords });
		}
	}

}
