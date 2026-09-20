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
#include "render/renderer/particle_renderer.h"
#include "asset/asset_manager.h"
#include "glm/gtc/matrix_transform.hpp"

namespace z1 {

	RendererForward::RendererForward() {
		m_default_material = g_runtime_context.m_asset_manager->get<MaterialInstance>(ENGINE_RESOURCE("material/MI_phone"));

		{
			Pipeline::Description desc{};
			desc.depth_test = true;
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>(ENGINE_RESOURCE("shader/skybox"));
			m_pipeline_skybox = Pipeline::build(desc);
		}

		// Initialize particle renderer
		m_particle_renderer.init();
	}

	RendererForward::~RendererForward() {

	}

	void RendererForward::draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer) {
		PROFILE_FUNCTION();

		auto const width = framebuffer->get_width();
		auto const height = framebuffer->get_height();

		auto& g = g_runtime_context.m_global;

		bool history_uninitialized = m_shared.ensure_buffers(width, height);

		int write_idx = m_shared.m_frame_index % 2;
		int read_idx = (m_shared.m_frame_index + 1) % 2;

		auto const& cam = scene->get_main_camera();

		auto& camera_comp = cam->get_component<CameraComponent>();
		if (!camera_comp.m_use_fixed_aspect) {
			camera_comp.m_aspect = framebuffer->get_aspect();
		}

		auto projview = camera_comp.get_proj() * camera_comp.get_view();
		auto cam_pos = camera_comp.get_position();

		auto proj_jittered = camera_comp.get_proj();
		if (g->taa_enabled) {
			float jx = halton(m_shared.m_frame_index + 1, 2) - 0.5f;
			float jy = halton(m_shared.m_frame_index + 1, 3) - 0.5f;

			float ndc_x = 2.0f * jx / width;
			float ndc_y = -2.0f * jy / height;

			proj_jittered[2][0] += ndc_x;
			proj_jittered[2][1] += ndc_y;

			// Pass jitter offset to TAA shader (in UV space: jx/width, jy/height)
			g->taa_jitter_uv = { jx / width, jy / height };
		}
		else {
			g->taa_jitter_uv = { 0.0f, 0.0f };
		}

		auto projview_jittered = proj_jittered * camera_comp.get_view();

		g->projview = projview_jittered;
		g->cam_position = cam_pos;

		// Own previous-frame matrix (the Global UBO may hold another scene's).
		g->prev_projview = m_shared.m_prev_projview;

		// Per-frame camera matrices for the AO passes
		m_shared.m_proj = camera_comp.get_proj();
		m_shared.m_inv_proj = glm::inverse(camera_comp.get_proj());
		m_shared.m_view = camera_comp.get_view();

		m_shared.update_lights(scene);
		m_shared.update_sky_light(scene);
		m_shared.calculate_csm_splits(camera_comp, g->sun_direction);

		g->flush();
		g->bind();

		Frustum frustum = create_frustum(projview);
		VisibleDrawList draw_list;
		auto& stats = g_runtime_context.m_graphics_context->m_stats;

		{
			auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const>();
			for (auto [entity, transform, mesh] : view.each()) {
				if (!mesh.m_mesh) continue;
				if (!is_mesh_visible(frustum, transform.get_world_transform(), mesh.m_mesh->m_bound_min, mesh.m_mesh->m_bound_max)) {
					stats.culled_objects++;
					continue;
				}

				draw_list.static_meshes.push_back({ transform.get_world_transform(), transform.m_prev_world_transform, &mesh });
				stats.visible_objects++;
			}
		}

		{
			auto view_skel = scene->m_registry.view<TransformComponent const, SkeletalMeshComponent const>();
			for (auto [entity, transform, mesh] : view_skel.each()) {
				if (!mesh.m_mesh) continue;

				glm::vec3 min = mesh.m_mesh->m_bound_min;
				glm::vec3 max = mesh.m_mesh->m_bound_max;

				AnimationComponent const* anim_comp = nullptr;
				if (scene->m_registry.all_of<AnimationComponent>(entity)) {
					anim_comp = &scene->m_registry.get<AnimationComponent>(entity);
					get_skeletal_bounds(mesh, *anim_comp, min, max);
				}

				if (!is_mesh_visible(frustum, transform.get_world_transform(), min, max)) {
					stats.culled_objects++;
					continue;
				}
				draw_list.skeletal_meshes.push_back({ transform.get_world_transform(), transform.m_prev_world_transform, &mesh, anim_comp });
				stats.visible_objects++;
			}
		}

		RenderGraph rg;
		rg.set_framebuffer_pool(m_shared.m_framebuffer_pool);
		int const cascade_count = std::min(std::max((int)g->sm_cascade_count, 1), MAX_CSM_CASCADES);
		m_shared.add_shadow_pass(rg, scene, m_default_material);
		m_particle_renderer.add_particle_shadow_passes(rg, scene.get(), m_shared.m_shadow_framebuffer, cascade_count);

		// Forward screen-space AO needs a depth+normal prepass before lighting.
		// Both are skipped entirely when AO is disabled.
		std::string ao_pass = "";
		if (g->ao_enabled) {
			m_shared.add_depth_prepass_pass(rg, draw_list, framebuffer, m_default_material);
			ao_pass = m_shared.add_ao_pass(rg, "prepass-depth", "prepass-normal");
		}

		// Forward MSAA: the main pass renders multisampled and a resolve republishes the
		// canonical single-sample scene-color/scene-depth for the rest of the chain.
		uint32_t const msaa_samples = m_shared.get_effective_msaa_samples();
		bool const msaa_on = msaa_samples > 1;
		std::string const main_passthrough = msaa_on ? "msaa-resolve-main" : "main";

		add_main_pass(rg, draw_list, scene, framebuffer, history_uninitialized, read_idx, projview, ao_pass, msaa_samples);
		if (msaa_on) {
			add_main_resolve_pass(rg, framebuffer);
		}
		m_particle_renderer.add_particle_pass(rg, scene.get(), main_passthrough, m_shared.m_shadow_image);

		// Final color chain: velocity/TAA/sharpen are skipped entirely when TAA is off and the
		// post-process pass then consumes the scene color directly.
		std::string final_color_input = "scene-color";
		if (g->taa_enabled) {
			m_shared.add_velocity_pass(rg, draw_list, scene, framebuffer, projview, m_default_material);
			m_shared.add_taa_pass(rg, m_shared.m_history_colors[write_idx], m_shared.m_history_colors[read_idx]);
			m_shared.add_taa_sharpen_pass(rg, m_shared.m_history_colors[write_idx]);
			final_color_input = "taa-sharpen";
		}

		bool const bloom_present = g->pp_bloom_enabled && !m_shared.m_bloom_textures.empty();
		if (bloom_present) {
			m_shared.add_bloom_pass(rg, final_color_input);
		}
		m_shared.add_postprocess_pass(rg, framebuffer, final_color_input, bloom_present);

		rg.compile();
		rg.execute();

		++m_shared.m_frame_index;
		g->unbind();
		g->prev_projview = projview;
		m_shared.m_prev_projview = projview;
	}

	void RendererForward::add_main_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer, bool history_uninitialized, int read_idx, glm::mat4 const& projview, std::string const& ao_pass, uint32_t samples) {
		bool const msaa_on = samples > 1;

		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::Clear;
		desc.color_attachments[0].clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Clear;
		desc.depth_stencil_attachment.clear_depth_value = 1.0f;

		auto const width = framebuffer->get_width();
		auto const height = framebuffer->get_height();

		auto& pass = rg.add_pass("main");
		pass.set_resolution_as(framebuffer)
			.set_pass_desc(desc)
			.add_output(msaa_on ? "scene-color-ms" : "scene-color", ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder, samples)
			.add_output(msaa_on ? "scene-depth-ms" : "scene-depth", ImageFormat::Depth, SamplerMode::Linear, WrapMode::Repeat, samples);

		// The AO texture is bound directly (not via add_input), so declare the
		// ordering explicitly to keep the AO pass ahead of the main pass.
		if (!ao_pass.empty()) {
			pass.depends_on(ao_pass);
		}

		pass.execute([this, &draw_list, scene, history_uninitialized, read_idx, width, height, projview](RenderGraphNode& node, GraphicsContext& ctx) {
				PerFrameConst per_frame{};
				per_frame.lights = m_shared.m_lights_buffer.get();
				per_frame.shadow_map = m_shared.m_shadow_image.get();
				per_frame.ao_map = m_shared.get_ao_image().get();
				m_shared.apply_sky_light(per_frame);

				// Pass 1: Opaque and Mask
				for (auto const& item : draw_list.static_meshes) {
					per_frame.model = item.transform;
					auto* overrides = item.mesh->override_materials_or_null();
					item.mesh->m_mesh->draw(per_frame, m_default_material, overrides, [](uint32_t flags) {
						return MaterialFlags::get_alpha_mode(flags) != AlphaMode::Blend;
					});
				}

				for (auto const& item : draw_list.skeletal_meshes) {
					std::shared_ptr<UniformBuffer> bones = nullptr;
					if (item.anim) {
						bones = item.anim->bone_ubo;
					}

					per_frame.model = item.transform;
					auto* overrides = item.mesh->override_materials_or_null();
					item.mesh->m_mesh->draw(per_frame, m_default_material, bones, overrides, [](uint32_t flags) {
						return MaterialFlags::get_alpha_mode(flags) != AlphaMode::Blend;
					});
				}

				// Pass 2: Blend
				for (auto const& item : draw_list.static_meshes) {
					per_frame.model = item.transform;
					auto* overrides = item.mesh->override_materials_or_null();
					item.mesh->m_mesh->draw(per_frame, m_default_material, overrides, [](uint32_t flags) {
						return MaterialFlags::get_alpha_mode(flags) == AlphaMode::Blend;
					});
				}

				for (auto const& item : draw_list.skeletal_meshes) {
					std::shared_ptr<UniformBuffer> bones = nullptr;
					if (item.anim) {
						bones = item.anim->bone_ubo;
					}

					per_frame.model = item.transform;
					auto* overrides = item.mesh->override_materials_or_null();
					item.mesh->m_mesh->draw(per_frame, m_default_material, bones, overrides, [](uint32_t flags) {
						return MaterialFlags::get_alpha_mode(flags) == AlphaMode::Blend;
					});
				}

				if (m_shared.m_has_sky_light && m_shared.m_sky_ibl_image) {
					m_pipeline_skybox->bind();

					auto& s = m_pipeline_skybox->m_shader;
					s->bind_texture(s->sampler_slot("u_sky_texture"), m_shared.m_sky_ibl_image.get());
					s->set_uniform("u_rotation", &m_shared.m_sky_rotation);
					s->set_uniform("u_intensity", &m_shared.m_sky_intensity);
					s->set_uniform("u_mip_level", &m_shared.m_sky_mip_level);

					auto& g = g_runtime_context.m_global;
					glm::mat4 inv_projview = glm::inverse(g->projview);
					s->set_uniform("u_inv_projview", &inv_projview);
					s->set_uniform("u_cam_position", &g->cam_position);

					m_shared.m_quad->bind();
					m_shared.m_quad->draw(PrimitiveType::Triangles);
					m_shared.m_quad->unbind();

					m_pipeline_skybox->unbind();
				}

				if (history_uninitialized) {
					ctx.blit_attachment(
						node.get_output(),
						m_shared.m_history_colors[read_idx],
						0, 0,
						0, 0,
						0, 0,
						width, height);
				}
				});
	}

	void RendererForward::add_main_resolve_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& framebuffer) {
		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::DontCare;
		// The depth resolve is a depth-writing draw (MS-to-SS depth blits are invalid).
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Clear;
		desc.depth_stencil_attachment.clear_depth_value = 1.0f;

		uint32_t const width = framebuffer->get_width();
		uint32_t const height = framebuffer->get_height();

		rg.add_pass("msaa-resolve-main")
			.set_resolution_as(framebuffer)
			.set_pass_desc(desc)
			.add_input("scene-color-ms")
			.add_input("scene-depth-ms")
			.add_output("scene-color", ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder)
			.add_output("scene-depth", ImageFormat::Depth)
			.pre_pass([width, height](RenderGraphNode& node, GraphicsContext& ctx) {
				auto src = node.get_input_framebuffer_name("scene-color-ms");
				auto dst = node.get_output();
				ctx.blit_attachment(src, dst, 0, 0, 0, 0, 0, 0, width, height);
			})
			.execute([this](RenderGraphNode& node, GraphicsContext& ctx) {
				m_shared.draw_msaa_depth_resolve(node, ctx, "scene-depth-ms");
			});
	}

}
