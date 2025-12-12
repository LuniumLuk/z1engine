#include "pch.h"
#include "render/shader.h"
#include "render/framebuffer.h"
#include "render/render_graph.h"
#include "render/graphics_context.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "scene/component/camera.h"
#include "scene/component/mesh.h"
#include "render/renderer/renderer_forward.h"
#include "asset/asset_manager.h"
#include "glm/gtc/matrix_transform.hpp"

namespace z1 {

	RendererForward::RendererForward() {
		m_default_material = g_runtime_context.m_asset_manager->get<MaterialInstance>(Guid::make("material/MI_phone"));
		m_global_buffer = UniformBuffer::create(nullptr, sizeof(GlobalConstants), BufferUsage::Dynamic);
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
			.set_output(framebuffer)
			.set_pass_desc(desc)
			.execute([&](RenderGraphNode& node, GraphicsContext& ctx) {
				auto const& cam = scene->get_main_camera();

				auto& camera_comp = cam->get_component<CameraComponent>();
				if (!camera_comp.m_use_fixed_aspect) {
					camera_comp.m_aspect = node.get_aspect();
				}

				auto projview = camera_comp.get_proj() * camera_comp.get_view();
				auto cam_pos = camera_comp.get_position();

				glm::vec3 sun_dir = { 0.577f, 0.577f, 0.577f };
				glm::vec3 sun_intensity = { .5f, .5f, .5f };
				PerFrameConst per_frame{};

				m_global_data.projview = projview;
				m_global_data.cam_position = glm::vec4(cam_pos, 0);
				m_global_data.sun_direction = glm::vec4(sun_dir, 0);
				m_global_data.sun_intensity = glm::vec4(sun_intensity, 0);

				m_global_buffer->write(&m_global_data, sizeof(GlobalConstants));
				m_global_buffer->bind();
				per_frame.global_binding = m_global_buffer->get_binding();

				auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const>();
				for (auto [entity, transform, mesh] : view.each()) {
					per_frame.model = transform.get_world_transform();
					mesh.m_mesh->draw(per_frame, m_default_material);
				}

				m_global_buffer->unbind();
				});

		rg.compile();
		rg.execute();

	}

}
