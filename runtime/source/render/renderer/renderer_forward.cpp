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

		Pipeline::Description desc{};
		desc.cull_mode = CullMode::None;
		desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/postprocessing");
		m_pipeline_postprocess = Pipeline::build(desc);
	}

	RendererForward::~RendererForward() {

	}

	void RendererForward::draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer) {
		PROFILE_FUNCTION();

		auto rg = RenderGraph();

		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::Clear;
		desc.color_attachments[0].clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Clear;
		desc.depth_stencil_attachment.clear_depth_value = 1.0f;

		rg.add_pass("main")
			.set_resolution_as(framebuffer)
			.set_pass_desc(desc)
			.add_output("scene-color", ImageFormat::RGBA32F)
			.add_output("scene-depth", ImageFormat::Depth)
			.execute([&](RenderGraphNode& node, GraphicsContext& ctx) {
				auto const& cam = scene->get_main_camera();

				auto& camera_comp = cam->get_component<CameraComponent>();
				if (!camera_comp.m_use_fixed_aspect) {
					camera_comp.m_aspect = node.get_aspect();
				}

				auto projview = camera_comp.get_proj() * camera_comp.get_view();
				auto cam_pos = camera_comp.get_position();

				auto& g = g_runtime_context.m_global;
				g->projview = projview;
				g->cam_position = cam_pos;
				g->flush();
				g->bind();

				PerFrameConst per_frame{};
				per_frame.global_binding = g_runtime_context.m_global->get_binding();

				auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const>();
				for (auto [entity, transform, mesh] : view.each()) {
					per_frame.model = transform.get_world_transform();
					mesh.m_mesh->draw(per_frame, m_default_material);
				}

				g->unbind();
				g->prev_projview = projview;
				});

		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::DontCare;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::DontCare;

		rg.add_pass("postprocessing")
			.set_output(framebuffer)
			.set_pass_desc(desc)
			.add_input("scene-color")
			.execute([&](RenderGraphNode& node, GraphicsContext& ctx) {
				auto& g = g_runtime_context.m_global;
				g->flush();
				g->bind();

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

				g->unbind();
				});

		rg.compile();
		rg.execute();

	}

}
