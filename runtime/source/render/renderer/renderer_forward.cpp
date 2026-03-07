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

#define CSM_LAYERS 4

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

		{
			Pipeline::Description desc{};
			desc.depth_test = true;
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/skybox");
			m_pipeline_skybox = Pipeline::build(desc);
		}

		// Shadow pipeline and framebuffer
		{
			Pipeline::Description desc{};
			desc.depth_test = true;
			desc.cull_mode = CullMode::Back;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/shadow");
			m_pipeline_shadow = Pipeline::build(desc);
			const uint32_t shadow_res = 2048;
			Framebuffer::Attachment attachment;
			attachment.format = ImageFormat::Depth;
			attachment.sampler_mode = SamplerMode::Nearest;
			attachment.wrap_mode = WrapMode::ClampToBorder;
			attachment.layers = 4;
			m_shadow_framebuffer = Framebuffer::create(shadow_res, shadow_res, { attachment });
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

		g->projview = projview_jittered;
		g->cam_position = cam_pos;

		update_lights(scene);
		calculate_csm_splits(camera_comp, g->sun_direction);

		g->flush();
		g->bind();

		RenderGraph rg;
		add_shadow_pass(rg, scene);
		add_main_pass(rg, scene, framebuffer, history_uninitialized, read_idx);
		add_velocity_pass(rg, scene, framebuffer, projview);
		add_taa_pass(rg, m_history_colors[write_idx], m_history_colors[read_idx]);
		add_postprocess_pass(rg, framebuffer, m_history_colors[write_idx]);

		rg.compile();
		rg.execute();

		++s_frame_index;
		g->unbind();
		g->prev_projview = projview;
	}

	void RendererForward::update_lights(std::shared_ptr<Scene> const& scene) {
		LightsBlock lights_block = {};
		int light_count = 0;

		auto lights = scene->m_registry.view<TransformComponent const, LightComponent const>();
		for (auto [entity, transform, light] : lights.each()) {
			if (light_count >= MAX_LIGHTS) break;

			glm::mat4 model = transform.get_world_transform();
			glm::vec3 pos = glm::vec3(model[3]);
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
	}

	void RendererForward::calculate_csm_splits(CameraComponent& camera, glm::vec3 const& sun_dir) {
		auto& g = g_runtime_context.m_global;
		float near_clip = camera.m_near;
		float far_clip = camera.m_far;
		float split_lambda = 0.95f;

		float splits[5] = {};
		splits[0] = near_clip;
		splits[4] = far_clip;

		for (int i = 1; i < 4; i++) {
			float p = (float)i / 4.0f;
			float log = near_clip * std::pow(far_clip / near_clip, p);
			float uniform = near_clip + (far_clip - near_clip) * p;
			splits[i] = split_lambda * log + (1.0f - split_lambda) * uniform;
		}
		g->csm_splits = glm::vec4(splits[1], splits[2], splits[3], splits[4]);

		glm::mat4 inv_view = glm::inverse(camera.get_view());
		glm::vec3 cam_pos_world = glm::vec3(inv_view[3]);
		glm::vec3 cam_forward = -glm::vec3(inv_view[2]);

		for (int i = 0; i < CSM_LAYERS; ++i) {
			float cascade_near = splits[i];
			float cascade_far = splits[i + 1];
			float mid_dist = (cascade_near + cascade_far) * 0.5f;

			glm::vec3 center = cam_pos_world + cam_forward * mid_dist;

			float fov = camera.m_intrinsic.fov;
			float aspect = camera.m_aspect;
			float tan_half_fov = std::tan(glm::radians(fov) * 0.5f);
			float far_height = 2.0f * cascade_far * tan_half_fov;
			float far_width = far_height * aspect;
			float diag = std::sqrt(far_height * far_height + far_width * far_width);

			float size = diag * 0.5f;

			glm::vec3 light_pos = center + glm::normalize(sun_dir) * size;

			glm::mat4 light_view = glm::lookAt(light_pos, center, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 light_proj = glm::ortho(-size, size, -size, size, -size * 6.0f, size * 6.0f);

			g->sun_projview[i] = light_proj * light_view;
		}
	}

	void RendererForward::add_shadow_pass(RenderGraph& rg, std::shared_ptr<Scene> const& scene) {
		auto& g = g_runtime_context.m_global;
		RenderPass::Description desc;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Clear;
		desc.depth_stencil_attachment.clear_depth_value = 1.0f;

		for (int cascade = 0; cascade < CSM_LAYERS; ++cascade) {
			rg.add_pass(std::string("shadow-CSM") + std::to_string(cascade))
				.set_output(m_shadow_framebuffer)
				.set_pass_desc(desc)
				.pre_pass([&, cascade](RenderGraphNode& node, GraphicsContext& ctx) {
					m_shadow_framebuffer->set_attachment_layer(0, cascade);
				})
				.execute([this, cascade, scene, &g](RenderGraphNode& node, GraphicsContext& ctx) {
					m_pipeline_shadow->bind();
					auto& s = m_pipeline_shadow->m_shader;
					s->set_uniform_block_binding("Global", g->get_binding());

					s->set_uniform("u_csm_index", &cascade);

					auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const>();
					for (auto [entity, transform, mesh] : view.each()) {
						glm::mat4 model = transform.get_world_transform();
						s->set_uniform("u_model", &model);
						mesh.m_mesh->draw();
					}

					auto view_skel =
						scene->m_registry.view<TransformComponent const, SkeletalMeshComponent const>();
					for (auto [entity, transform, mesh] : view_skel.each()) {
						if (!mesh.m_mesh)
							continue;
						glm::mat4 model = transform.get_world_transform();
						s->set_uniform("u_model", &model);
						int has_skinning = 0;
						if (scene->m_registry.all_of<AnimationComponent>(entity)) {
							auto const& anim = scene->m_registry.get<AnimationComponent>(entity);
							if (anim.bone_ubo) {
								has_skinning = 1;
								anim.bone_ubo->bind();
								s->set_uniform_block_binding("Bones", anim.bone_ubo->get_binding());
							}
						}
						s->set_uniform("u_has_skinning", &has_skinning);
						mesh.m_mesh->draw();
						if (has_skinning)
							scene->m_registry.get<AnimationComponent>(entity).bone_ubo->unbind();
					}
					m_pipeline_shadow->unbind();
				});
		}
	}

	void RendererForward::add_main_pass(RenderGraph& rg, std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer, bool history_uninitialized, int read_idx) {
		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::Clear;
		desc.color_attachments[0].clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Clear;
		desc.depth_stencil_attachment.clear_depth_value = 1.0f;

		auto const width = framebuffer->get_width();
		auto const height = framebuffer->get_height();

		rg.add_pass("main")
			.set_resolution_as(framebuffer)
			.set_pass_desc(desc)
			.add_output("scene-color", ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder)
			.add_output("scene-depth", ImageFormat::Depth)
			.execute([this, scene, history_uninitialized, read_idx, width, height](RenderGraphNode& node, GraphicsContext& ctx) {
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

					std::shared_ptr<UniformBuffer> bones = nullptr;
					if (scene->m_registry.all_of<AnimationComponent>(entity)) {
						auto const& anim = scene->m_registry.get<AnimationComponent>(entity);
						bones = anim.bone_ubo;
					}
					mesh.m_mesh->draw(per_frame, m_default_material, bones);
				}

				auto sky_view = scene->m_registry.view<SkyLightComponent const>();
				for (auto [entity, sky] : sky_view.each()) {
					if (sky.m_texture && sky.m_texture->m_image) {
						m_pipeline_skybox->bind();

						sky.m_texture->m_image->bind(m_pipeline_skybox->m_shader, "u_sky_texture");

						auto& s = m_pipeline_skybox->m_shader;
						s->set_uniform("u_rotation", &sky.m_rotation);
						s->set_uniform("u_intensity", &sky.m_intensity);
						s->set_uniform("u_mip_level", &sky.m_mip_level);

						auto& g = g_runtime_context.m_global;
						glm::mat4 inv_projview = glm::inverse(g->projview);
						s->set_uniform("u_inv_projview", &inv_projview);
						s->set_uniform("u_cam_position", &g->cam_position);

						m_quad->bind();
						m_quad->draw(PrimitiveType::Triangles);
						m_quad->unbind();

						sky.m_texture->m_image->unbind();
						m_pipeline_skybox->unbind();
					}
					break;
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
	}

	void RendererForward::add_velocity_pass(RenderGraph& rg, std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer, glm::mat4 const& projview) {
		auto& g = g_runtime_context.m_global;
		RenderPass::Description desc;
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
			.execute([this, scene, projview, &g](RenderGraphNode& node, GraphicsContext& ctx) {

				auto jittered_projview = g->projview;
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
						if (anim.bone_ubo) {
							has_skinning = 1;
							anim.bone_ubo->bind();
							s->set_uniform_block_binding("Bones", anim.bone_ubo->get_binding());
						}
					}
					s->set_uniform("u_has_skinning", &has_skinning);
					mesh.m_mesh->draw();
					if (has_skinning)
						scene->m_registry.get<AnimationComponent>(entity).bone_ubo->unbind();
				}

				m_pipeline_velocity->unbind();

				g->projview = jittered_projview;
				g->flush();

				});
	}

	void RendererForward::add_taa_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& history_write, std::shared_ptr<Framebuffer> const& history_read) {
		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::DontCare;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::DontCare;

		rg.add_pass("taa")
			.set_output(history_write)
			.set_pass_desc(desc)
			.add_input("scene-color")
			.add_input("velocity")
			.execute([this, history_read](RenderGraphNode& node, GraphicsContext& ctx) {
				auto& h = history_read->get_attachment_image(0);
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
	}

	void RendererForward::add_postprocess_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& target, std::shared_ptr<Framebuffer> const& source) {
		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::DontCare;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::DontCare;

		rg.add_pass("postprocessing")
			.set_output(target)
			.set_pass_desc(desc)
			.depends_on("taa")
			.execute([this, source](RenderGraphNode& node, GraphicsContext& ctx) {
				m_pipeline_postprocess->bind();
				auto& s = m_pipeline_postprocess->m_shader;
				s->set_uniform_block_binding(
					"Global",
					g_runtime_context.m_global->get_binding());

				auto& scene = source->get_attachment_image(0);
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
	}

}
