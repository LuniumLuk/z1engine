#include "pch.h"
#include "render/global.h"
#include "render/shader.h"
#include "render/framebuffer.h"
#include "render/render_graph.h"
#include "render/graphics_context.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "scene/component/camera.h"
#include "scene/component/mesh.h"
#include "scene/component/sprite.h"
#include "render/renderer/renderer_forward.h"
#include "asset/asset_manager.h"
#include "glm/gtc/matrix_transform.hpp"

namespace z1 {

	static float halton(int index, int base) {
		float f = 1.0f;
		float r = 0.0f;
		while (index > 0)
		{
			f /= base;
			r += f * (index % base);
			index /= base;
		}
		return r;
	}

	static int s_frame_index = 0;

	RendererForward::RendererForward() {
		m_default_material = g_runtime_context.m_asset_manager->get<MaterialInstance>(Guid::make("material/MI_phone"));

		std::vector<float> vertices = {
			// first triangle
			-1.0f, -1.0f,
			 1.0f, -1.0f,
			 1.0f,  1.0f,

			 // second triangle
			 -1.0f, -1.0f,
			  1.0f,  1.0f,
			 -1.0f,  1.0f
		};
		auto vbo = VertexBuffer::create(
			vertices.data(), vertices.size() * sizeof(float),
			{
				{ DataType::Float2 },
			});
		m_quad = VertexArray::create({ vbo }, nullptr);

		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/postprocessing");
			m_pipeline_postprocess = Pipeline::build(desc);
		}

		{
			Pipeline::Description desc{};
			desc.depth_test = true;
			desc.blend = true;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/velocity");
			m_pipeline_velocity = Pipeline::build(desc);
		}

		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/taa");
			m_pipeline_taa = Pipeline::build(desc);
		}

		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/copy");
			m_pipeline_copy = Pipeline::build(desc);
		}
	}

	RendererForward::~RendererForward() {

	}

	void RendererForward::draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer) {
		PROFILE_FUNCTION();

		auto const width = framebuffer->get_width();
		auto const height = framebuffer->get_height();

		auto& g = g_runtime_context.m_global;

		bool history_uninitialized = false;

		if (!m_history_color) {
			m_history_color = Framebuffer::create(
				width,
				height,
				{ { ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder } });
			history_uninitialized = true;
		}

		if (m_history_color->get_width() != width ||
			m_history_color->get_height() != height) {
			m_history_color->resize(width, height);
			history_uninitialized = true;
		}

		auto const& cam = scene->get_main_camera();

		auto& camera_comp = cam->get_component<CameraComponent>();
		if (!camera_comp.m_use_fixed_aspect) {
			camera_comp.m_aspect = framebuffer->get_aspect();
		}

		auto projview = camera_comp.get_proj() * camera_comp.get_view();
		auto cam_pos = camera_comp.get_position();

		auto proj_jittered = camera_comp.get_proj();
		if (g->taa_enabled) {
			// jittered projection matrix for TAA
			float jx = halton(s_frame_index + 1, 2) - 0.5f;
			float jy = halton(s_frame_index + 1, 3) - 0.5f;

			float ndc_x = 2.0f * jx / width;
			float ndc_y = -2.0f * jy / height;

			proj_jittered[2][0] += ndc_x;
			proj_jittered[2][1] += ndc_y;
		}

		auto projview_jittered = proj_jittered * camera_comp.get_view();

		auto rg = RenderGraph();

		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::Clear;
		desc.color_attachments[0].clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Clear;
		desc.depth_stencil_attachment.clear_depth_value = 1.0f;

		g->projview = projview_jittered;
		g->cam_position = cam_pos;
		g->flush();
		g->bind();

		rg.add_pass("main")
			.set_resolution_as(framebuffer)
			.set_pass_desc(desc)
			.add_output("scene-color", ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder)
			.add_output("scene-depth", ImageFormat::Depth)
			.execute([&](RenderGraphNode& node, GraphicsContext& ctx) {
				PerFrameConst per_frame{};
				per_frame.global_binding = g_runtime_context.m_global->get_binding();

				auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const>();
				for (auto [entity, transform, mesh] : view.each()) {
					per_frame.model = transform.get_world_transform();
					mesh.m_mesh->draw(per_frame, m_default_material);
				}

				if (history_uninitialized) {
					ctx.blit_attachment(
						node.get_output(),
						m_history_color,
						0, 0,
						0, 0,
						0, 0,
						width, height);
				}
				});

		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::Clear;
		desc.color_attachments[0].clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Clear;
		desc.depth_stencil_attachment.clear_depth_value = 1.0f;

		rg.add_pass("velocity")
			.set_resolution_as(framebuffer)
			.set_pass_desc(desc)
			.add_output("velocity", ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder)
			.add_output("velocity-depth", ImageFormat::Depth)
			.execute([&](RenderGraphNode& node, GraphicsContext& ctx) {
			auto const& cam = scene->get_main_camera();
				g->projview = projview;
				g->flush();

				m_pipeline_velocity->bind();
				auto& s = m_pipeline_velocity->m_shader;
				s->set_uniform_block_binding("Global", g->get_binding());

				auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const>();
				for (auto [entity, transform, mesh] : view.each()) {
					s->set_uniform("u_model", &transform.get_world_transform());
					mesh.m_mesh->draw();
				}

				m_pipeline_velocity->unbind();
				});

		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::DontCare;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::DontCare;

		rg.add_pass("taa")
			.set_resolution_as(framebuffer)
			.set_pass_desc(desc)
			.add_output("taa", ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder)
			.add_input("scene-color")
			.add_input("velocity")
			.execute([&](RenderGraphNode& node, GraphicsContext& ctx) {
				auto& h = m_history_color->get_attachment_image(0);
				h->bind();

				m_pipeline_taa->bind();
				auto& s = m_pipeline_taa->m_shader;
				s->set_uniform_block_binding(
					"Global",
					g_runtime_context.m_global->get_binding());
				s->set_uniform_binding(
					"u_current_color",
					node.bind_input_index(0));
				s->set_uniform_binding(
					"u_history_color",
					h->get_binding());
				s->set_uniform_binding(
					"u_velocity",
					node.bind_input_index(1));

				m_quad->bind();
				m_quad->draw(PrimitiveType::Triangles);
				m_quad->unbind();

				node.unbind_input_index(0);
				node.unbind_input_index(1);
				h->unbind();

				m_pipeline_taa->unbind();
				});

		rg.add_pass("copy")
			.set_output(m_history_color)
			.set_pass_desc(desc)
			.add_input("taa")
			.execute([&](RenderGraphNode& node, GraphicsContext& ctx) {
				m_pipeline_copy->bind();
				auto& s = m_pipeline_copy->m_shader;
				s->set_uniform_binding(
					"u_input",
					node.bind_input_index(0));

				m_quad->bind();
				m_quad->draw(PrimitiveType::Triangles);
				m_quad->unbind();

				node.unbind_input_index(0);

				m_pipeline_copy->unbind();
				});

		rg.add_pass("postprocessing")
			.set_output(framebuffer)
			.set_pass_desc(desc)
			.add_input("taa")
			.execute([&](RenderGraphNode& node, GraphicsContext& ctx) {
				m_pipeline_postprocess->bind();
				auto& s = m_pipeline_postprocess->m_shader;
				s->set_uniform_block_binding(
					"Global",
					g_runtime_context.m_global->get_binding());
				s->set_uniform_binding(
					"u_scene",
					node.bind_input_index(0));

				m_quad->bind();
				m_quad->draw(PrimitiveType::Triangles);
				m_quad->unbind();

				node.unbind_input_index(0);

				m_pipeline_postprocess->unbind();
				});

		rg.compile();
		rg.execute();

		++s_frame_index;
		g->unbind();
		g->prev_projview = projview;

	}

}
