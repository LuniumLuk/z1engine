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
#include "scene/component/light.h"
#include "scene/component/animation.h"
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

		/*
		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/copy");
			m_pipeline_copy = Pipeline::build(desc);
		}
		*/

		// Shadow pipeline and framebuffer
		{
			Pipeline::Description desc{};
			desc.depth_test = true;
			desc.cull_mode = CullMode::Back;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/shadow");
			m_pipeline_shadow = Pipeline::build(desc);
			const uint32_t shadow_res = 2048;
			m_shadow_framebuffer = Framebuffer::create(shadow_res, shadow_res, { { ImageFormat::Depth, SamplerMode::Nearest, WrapMode::ClampToBorder } });
			// cache the depth attachment image for binding
			m_shadow_image = m_shadow_framebuffer->get_attachment_image(0);
		}

		m_lights_buffer = UniformBuffer::create(nullptr, sizeof(LightsBlock), BufferUsage::Static);
	}

	RendererForward::~RendererForward() {

	}

	void RendererForward::draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer) {
		PROFILE_FUNCTION();

		auto const width = framebuffer->get_width();
		auto const height = framebuffer->get_height();

		auto& g = g_runtime_context.m_global;

		bool history_uninitialized = false;

		if (!m_history_colors[0]) {
			m_history_colors[0] = Framebuffer::create(
				width,
				height,
				{ { ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder } });
			m_history_colors[1] = Framebuffer::create(
				width,
				height,
				{ { ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder } });
			history_uninitialized = true;
		}

		if (m_history_colors[0]->get_width() != width ||
			m_history_colors[0]->get_height() != height) {
			m_history_colors[0]->resize(width, height);
			m_history_colors[1]->resize(width, height);
			history_uninitialized = true;
		}

		int write_idx = s_frame_index % 2;
		int read_idx = (s_frame_index + 1) % 2;

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

		// --- Light Logic ---
		LightsBlock lights_block = {};
		int light_count = 0;
		bool found_shadow_caster = false;

		auto lights = scene->m_registry.view<TransformComponent const, LightComponent const>();
		for (auto [entity, transform, light] : lights.each()) {
			if (light_count >= MAX_LIGHTS) break;

			glm::mat4 model = transform.get_world_transform();
			glm::vec3 pos = glm::vec3(model[3]);
			// forward direction (-Z in local space) transformed to world space
			// use rotation part only
			glm::mat3 rot = glm::mat3(model);
			glm::vec3 direction = glm::normalize(rot * glm::vec3(0, 0, -1));

			lights_block.lights[light_count].position = glm::vec4(pos, (float)light.m_type);
			lights_block.lights[light_count].direction = glm::vec4(direction, light.m_range);
			lights_block.lights[light_count].color = glm::vec4(light.m_color, light.m_intensity);
			lights_block.lights[light_count].cone = glm::vec4(
				glm::cos(glm::radians(light.m_inner_cone)),
				glm::cos(glm::radians(light.m_outer_cone)),
				light.m_cast_shadow ? 1.0f : 0.0f,
				0.0f
			);

			light_count++;
		}
		lights_block.count.x = (float)light_count;
		m_lights_buffer->write(&lights_block, sizeof(LightsBlock));

		// compute light projection (simple orthographic)
		float ortho_size = g->sm_ortho_size;
		glm::vec3 sun_dir = g->sun_direction;
		glm::vec3 light_pos = cam_pos + glm::normalize(sun_dir) * ortho_size;
		glm::mat4 light_view = glm::lookAt(light_pos, cam_pos, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 light_proj = glm::ortho(-ortho_size, ortho_size, -ortho_size, ortho_size, g->sm_near, g->sm_far);
		g->sun_projview = light_proj * light_view;

		g->flush();
		g->bind();

		// Shadow pass: render depth from sun's POV into a dedicated framebuffer
		rg.add_pass("shadow")
			.set_output(m_shadow_framebuffer)
			.set_pass_desc(desc)
			.execute([&](RenderGraphNode& node, GraphicsContext& ctx) {
				m_pipeline_shadow->bind();
				auto& s = m_pipeline_shadow->m_shader;
				s->set_uniform_block_binding("Global", g->get_binding());
				auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const>();
				for (auto [entity, transform, mesh] : view.each()) {
					glm::mat4 model = transform.get_world_transform();
					s->set_uniform("u_model", &model);
					// static shader doesn't have skinning uniform
					mesh.m_mesh->draw();
				}

				auto view_skel = scene->m_registry.view<TransformComponent const, SkeletalMeshComponent const>();
				for (auto [entity, transform, mesh] : view_skel.each()) {
					if (!mesh.m_mesh) continue;
					glm::mat4 model = transform.get_world_transform();
					s->set_uniform("u_model", &model);
					int has_skinning = 0;
					if (scene->m_registry.all_of<AnimationComponent>(entity)) {
						auto const& anim = scene->m_registry.get<AnimationComponent>(entity);
						if (!anim.bone_matrices.empty()) {
							has_skinning = 1;
							s->set_uniform("u_bone_matrices", anim.bone_matrices.data());
						}
					}
					s->set_uniform("u_has_skinning", &has_skinning);
					mesh.m_mesh->draw();
				}
				m_pipeline_shadow->unbind();
			});

		rg.add_pass("main")
			.set_resolution_as(framebuffer)
			.set_pass_desc(desc)
			.add_output("scene-color", ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder)
			.add_output("scene-depth", ImageFormat::Depth)
			.execute([&](RenderGraphNode& node, GraphicsContext& ctx) {
				PerFrameConst per_frame{};
				per_frame.global_binding = g_runtime_context.m_global->get_binding();

				m_lights_buffer->bind();
				per_frame.lights_binding = m_lights_buffer->get_binding();

				auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const>();
				for (auto [entity, transform, mesh] : view.each()) {
					if (!mesh.m_mesh) continue;
					per_frame.model = transform.get_world_transform();
					mesh.m_mesh->draw(per_frame, m_default_material);
				}

				auto view_skel = scene->m_registry.view<TransformComponent const, SkeletalMeshComponent const>();
				for (auto [entity, transform, mesh] : view_skel.each()) {
					if (!mesh.m_mesh) continue;
					per_frame.model = transform.get_world_transform();

					std::vector<glm::mat4> const* bones = nullptr;
					if (scene->m_registry.all_of<AnimationComponent>(entity)) {
						auto const& anim = scene->m_registry.get<AnimationComponent>(entity);
						bones = &anim.bone_matrices;
					}
					mesh.m_mesh->draw(per_frame, m_default_material, bones);
				}

				if (history_uninitialized) {
					ctx.blit_attachment(
						node.get_output(),
						m_history_colors[read_idx],
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

				auto view_skel = scene->m_registry.view<TransformComponent const, SkeletalMeshComponent const>();
				for (auto [entity, transform, mesh] : view_skel.each()) {
					if (!mesh.m_mesh) continue;
					s->set_uniform("u_model", &transform.get_world_transform());
					int has_skinning = 0;
					if (scene->m_registry.all_of<AnimationComponent>(entity)) {
						auto const& anim = scene->m_registry.get<AnimationComponent>(entity);
						if (!anim.bone_matrices.empty()) {
							has_skinning = 1;
							s->set_uniform("u_bone_matrices", anim.bone_matrices.data());
						}
					}
					s->set_uniform("u_has_skinning", &has_skinning);
					mesh.m_mesh->draw();
				}

				m_pipeline_velocity->unbind();
				});

		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::DontCare;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::DontCare;

		rg.add_pass("taa")
			.set_output(m_history_colors[write_idx])
			.set_pass_desc(desc)
			.add_input("scene-color")
			.add_input("velocity")
			.execute([&](RenderGraphNode& node, GraphicsContext& ctx) {
				auto& h = m_history_colors[read_idx]->get_attachment_image(0);
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

		/*
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
		*/

		rg.add_pass("postprocessing")
			.set_output(framebuffer)
			.set_pass_desc(desc)
			// .add_input("taa")
			.execute([&](RenderGraphNode& node, GraphicsContext& ctx) {
				m_pipeline_postprocess->bind();
				auto& s = m_pipeline_postprocess->m_shader;
				s->set_uniform_block_binding(
					"Global",
					g_runtime_context.m_global->get_binding());

				auto& scene = m_history_colors[write_idx]->get_attachment_image(0);
				scene->bind();
				s->set_uniform_binding(
					"u_scene",
					scene->get_binding());

				m_quad->bind();
				m_quad->draw(PrimitiveType::Triangles);
				m_quad->unbind();

				scene->unbind();

				m_pipeline_postprocess->unbind();
				});

		rg.compile();
		rg.execute();

		++s_frame_index;
		g->unbind();
		g->prev_projview = projview;

	}

}
