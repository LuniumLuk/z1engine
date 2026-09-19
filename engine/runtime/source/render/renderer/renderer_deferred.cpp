#include "pch.h"
#include "render/global.h"
#include "render/shader.h"
#include "render/shader_variant.h"
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
#include "render/renderer/renderer_deferred.h"
#include "render/renderer/particle_renderer.h"
#include "asset/asset_manager.h"
#include "render/uniform_blocks.h"
#include "glm/gtc/matrix_transform.hpp"

namespace z1 {

	RendererDeferred::RendererDeferred() {
		m_default_material = g_runtime_context.m_asset_manager->get<MaterialInstance>(ENGINE_RESOURCE("material/MI_phone"));

		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>(ENGINE_RESOURCE("shader/deferred_lighting"));
			m_pipeline_deferred_lighting = Pipeline::build(desc);
		}

		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>(ENGINE_RESOURCE("shader/ssr"));
			m_pipeline_ssr = Pipeline::build(desc);
		}

		{
			Pipeline::Description desc{};
			desc.depth_test = true;
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>(ENGINE_RESOURCE("shader/deferred_skybox"));
			m_pipeline_skybox = Pipeline::build(desc);
		}

		// Initialize particle renderer
		m_particle_renderer.init();
	}

	RendererDeferred::~RendererDeferred() {

	}

	void RendererDeferred::draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer) {
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

		// Per-frame camera matrices for the AO passes (unjittered is fine: the TAA
		// jitter only shifts NDC xy, not the depth used for reconstruction)
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
		add_gbuffer_pass(rg, draw_list, framebuffer, projview);
		std::string ao_pass = m_shared.add_ao_pass(rg, "gbuffer-depth", "gbuffer-normal");
		add_deferred_lighting_pass(rg, framebuffer, history_uninitialized, read_idx, ao_pass);

		std::string scene_color_input = "scene-color";
		std::string passthrough_pass = "deferred-lighting";
		if (g->ssr_enabled) {
			add_ssr_pass(rg, framebuffer);
			scene_color_input = "scene-color-ssr";
			passthrough_pass = "ssr";
		}

		add_forward_transparency_pass(rg, framebuffer, draw_list, scene, passthrough_pass, ao_pass);
		m_particle_renderer.add_particle_pass(rg, scene.get(), "forward-transparency", m_shared.m_shadow_image);

		// Final color chain: velocity/TAA/sharpen are skipped entirely when TAA is off and the
		// post-process pass then consumes the scene color (or SSR output) directly.
		std::string final_color_input = scene_color_input;
		if (g->taa_enabled) {
			m_shared.add_velocity_pass(rg, draw_list, scene, framebuffer, projview, m_default_material);
			m_shared.add_taa_pass(rg, m_shared.m_history_colors[write_idx], m_shared.m_history_colors[read_idx], scene_color_input);
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

	void RendererDeferred::ensure_lighting_slots() {
		if (m_lighting_slots.m_valid) {
			return;
		}
		auto const& lighting = m_pipeline_deferred_lighting->m_shader;
		m_lighting_slots.m_shadow = lighting->sampler_slot("u_shadow_map");
		m_lighting_slots.m_ao = lighting->sampler_slot("u_ao_texture");
		m_lighting_slots.m_sky_ibl = lighting->sampler_slot("u_sky_ibl_texture");
		m_lighting_slots.m_gbuffer_position = lighting->sampler_slot("u_gbuffer_position");
		m_lighting_slots.m_gbuffer_normal = lighting->sampler_slot("u_gbuffer_normal");
		m_lighting_slots.m_gbuffer_albedo = lighting->sampler_slot("u_gbuffer_albedo");
		m_lighting_slots.m_gbuffer_mr = lighting->sampler_slot("u_gbuffer_metallic_roughness");
		m_lighting_slots.m_gbuffer_emissive = lighting->sampler_slot("u_gbuffer_emissive");

		auto const& ssr = m_pipeline_ssr->m_shader;
		m_lighting_slots.m_ssr_scene = ssr->sampler_slot("u_scene_color");
		m_lighting_slots.m_ssr_position = ssr->sampler_slot("u_gbuffer_position");
		m_lighting_slots.m_ssr_normal = ssr->sampler_slot("u_gbuffer_normal");
		m_lighting_slots.m_ssr_albedo = ssr->sampler_slot("u_gbuffer_albedo");
		m_lighting_slots.m_ssr_mr = ssr->sampler_slot("u_gbuffer_metallic_roughness");
		m_lighting_slots.m_ssr_depth = ssr->sampler_slot("u_gbuffer_depth");
		m_lighting_slots.m_valid = true;
	}

	// G-buffer pass
	// Renders opaque + masked geometry to a multi-render-target FBO:
	//   RT0: position  (RGB16F)
	//   RT1: normal    (RGB16F)
	//   RT2: albedo    (RGBA8)
	//   RT3: metallic-roughness (RG16F)
	//   DS : depth

	void RendererDeferred::add_gbuffer_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Framebuffer> const& framebuffer, glm::mat4 const& unjittered_projview) {
		RenderPass::Description desc;
		desc.color_attachments.resize(5);
		for (int i = 0; i < 5; i++) {
			desc.color_attachments[i].load_op = LoadOp::Clear;
			desc.color_attachments[i].clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };
		}
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Clear;
		desc.depth_stencil_attachment.clear_depth_value = 1.0f;

		rg.add_pass("gbuffer")
			.set_resolution_as(framebuffer)
			.set_pass_desc(desc)
			.add_output("gbuffer-position", ImageFormat::RGBA32F, SamplerMode::Nearest, WrapMode::ClampToEdge)
			.add_output("gbuffer-normal", ImageFormat::RGB16F, SamplerMode::Nearest, WrapMode::ClampToEdge)
			.add_output("gbuffer-albedo", ImageFormat::RGBA8, SamplerMode::Nearest, WrapMode::ClampToEdge)
			.add_output("gbuffer-metallic-roughness", ImageFormat::RG16F, SamplerMode::Nearest, WrapMode::ClampToEdge)
			.add_output("gbuffer-emissive", ImageFormat::RGB16F, SamplerMode::Nearest, WrapMode::ClampToEdge)
			.add_output("gbuffer-depth", ImageFormat::Depth)
			.execute([this, &draw_list, unjittered_projview](RenderGraphNode& node, GraphicsContext& ctx) {
				PerFrameConst per_frame{};
				per_frame.variant_key = ShaderVariant::GBuffer;
				per_frame.lights = m_shared.m_lights_buffer.get();

				// Render opaque and masked geometry only
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

				// Render skybox to emissive channel of G-buffer.
				if (m_shared.m_has_sky_light && m_shared.m_sky_ibl_image) {
					m_pipeline_skybox->bind();

					auto& s = m_pipeline_skybox->m_shader;
					s->bind_texture(s->sampler_slot("u_sky_texture"), m_shared.m_sky_ibl_image.get());
					s->set_uniform("u_rotation", &m_shared.m_sky_rotation);
					s->set_uniform("u_intensity", &m_shared.m_sky_intensity);
					s->set_uniform("u_mip_level", &m_shared.m_sky_mip_level);

					auto& g = g_runtime_context.m_global;
					// Use unjittered projview so the skybox is stable across TAA frames
					glm::mat4 inv_projview = glm::inverse(unjittered_projview);
					s->set_uniform("u_inv_projview", &inv_projview);
					s->set_uniform("u_cam_position", &g->cam_position);

					m_shared.m_quad->bind();
					m_shared.m_quad->draw(PrimitiveType::Triangles);
					m_shared.m_quad->unbind();

					m_pipeline_skybox->unbind();
				}
			});
	}

	// Deferred lighting pass
	// Fullscreen quad that reads G-buffer + shadow map + lights UBO
	// and writes lit scene-color.

	void RendererDeferred::add_deferred_lighting_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& framebuffer, bool history_uninitialized, int read_idx, std::string const& ao_pass) {
		ensure_lighting_slots();
		auto const width = framebuffer->get_width();
		auto const height = framebuffer->get_height();

		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::Clear;
		desc.color_attachments[0].clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Load;

		auto& pass = rg.add_pass("deferred-lighting");
		pass.set_resolution_as(framebuffer)
			.set_pass_desc(desc)
			.add_input("gbuffer-position")
			.add_input("gbuffer-normal")
			.add_input("gbuffer-albedo")
			.add_input("gbuffer-metallic-roughness")
			.add_input("gbuffer-emissive")
			.add_input("gbuffer-depth")
			.add_output("scene-color", ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder)
			.add_output("scene-depth", ImageFormat::Depth)
			.pre_pass([](RenderGraphNode& node, GraphicsContext& ctx) {
				auto src = node.get_input_framebuffer_name("gbuffer-depth");
				auto dst = node.get_output();
				ctx.blit_depth_stencil(src, dst);
			});

		// The AO texture is bound directly (not via add_input), so declare the
		// ordering explicitly to keep the AO pass ahead of lighting.
		if (!ao_pass.empty()) {
			pass.depends_on(ao_pass);
		}

		pass.execute([this, history_uninitialized, read_idx, width, height](RenderGraphNode& node, GraphicsContext& ctx) {
			m_pipeline_deferred_lighting->bind();
			auto& s = m_pipeline_deferred_lighting->m_shader;

			auto& g = g_runtime_context.m_global;
			ctx.bind_uniform_buffer(uniform_blocks::Global, g->get_buffer());
			ctx.bind_uniform_buffer(uniform_blocks::Lights, *m_shared.m_lights_buffer);

			s->bind_texture(m_lighting_slots.m_shadow, m_shared.m_shadow_image.get());
			s->bind_texture(m_lighting_slots.m_ao, m_shared.get_ao_image().get());
			s->bind_texture(m_lighting_slots.m_sky_ibl,
				m_shared.m_has_sky_light ? m_shared.m_sky_ibl_image.get() : nullptr);

			// Bind G-buffer textures
			node.bind_input(s, m_lighting_slots.m_gbuffer_position, "gbuffer-position");
			node.bind_input(s, m_lighting_slots.m_gbuffer_normal, "gbuffer-normal");
			node.bind_input(s, m_lighting_slots.m_gbuffer_albedo, "gbuffer-albedo");
			node.bind_input(s, m_lighting_slots.m_gbuffer_mr, "gbuffer-metallic-roughness");
			node.bind_input(s, m_lighting_slots.m_gbuffer_emissive, "gbuffer-emissive");

			m_shared.m_quad->bind();
			m_shared.m_quad->draw(PrimitiveType::Triangles);
			m_shared.m_quad->unbind();

			m_pipeline_deferred_lighting->unbind();

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

	void RendererDeferred::add_ssr_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& framebuffer) {
		ensure_lighting_slots();
		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::DontCare;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Load;

		rg.add_pass("ssr")
			.set_resolution_as(framebuffer)
			.set_pass_desc(desc)
			.add_input("scene-color")
			.add_input("gbuffer-position")
			.add_input("gbuffer-normal")
			.add_input("gbuffer-albedo")
			.add_input("gbuffer-metallic-roughness")
			.add_input("gbuffer-depth")
			.add_output("scene-color-ssr", ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder)
			.add_output("scene-depth-ssr", ImageFormat::Depth)
			.pre_pass([](RenderGraphNode& node, GraphicsContext& ctx) {
				auto src = node.get_input_framebuffer_name("gbuffer-depth");
				auto dst = node.get_output();
				ctx.blit_depth_stencil(src, dst);
			})
			.depends_on("deferred-lighting")
			.execute([this](RenderGraphNode& node, GraphicsContext& ctx) {
				m_pipeline_ssr->bind();
				auto& s = m_pipeline_ssr->m_shader;

				auto& g = g_runtime_context.m_global;
				ctx.bind_uniform_buffer(uniform_blocks::Global, g->get_buffer());

				node.bind_input(s, m_lighting_slots.m_ssr_scene, "scene-color");
				node.bind_input(s, m_lighting_slots.m_ssr_position, "gbuffer-position");
				node.bind_input(s, m_lighting_slots.m_ssr_normal, "gbuffer-normal");
				node.bind_input(s, m_lighting_slots.m_ssr_albedo, "gbuffer-albedo");
				node.bind_input(s, m_lighting_slots.m_ssr_mr, "gbuffer-metallic-roughness");
				node.bind_input(s, m_lighting_slots.m_ssr_depth, "gbuffer-depth");

				m_shared.m_quad->bind();
				m_shared.m_quad->draw(PrimitiveType::Triangles);
				m_shared.m_quad->unbind();

				m_pipeline_ssr->unbind();
			});
	}

	// Forward transparency pass
	// Blended objects cannot be deferred. Render them on top of the
	// lit scene-color using normal forward shading + skybox.

	void RendererDeferred::add_forward_transparency_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& framebuffer, VisibleDrawList const& draw_list, std::shared_ptr<Scene> const& scene, std::string const& input_pass, std::string const& ao_pass) {
		ensure_lighting_slots();
		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::Load;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Load;

		auto& pass = rg.add_pass("forward-transparency");
		pass.set_resolution_as(framebuffer)
			.set_pass_desc(desc)
			.set_passthrough(input_pass);

		if (!ao_pass.empty()) {
			pass.depends_on(ao_pass);
		}

		pass.execute([this, &draw_list, scene](RenderGraphNode& node, GraphicsContext& ctx) {
			PerFrameConst per_frame{};
			per_frame.lights = m_shared.m_lights_buffer.get();
			per_frame.shadow_map = m_shared.m_shadow_image.get();
			per_frame.ao_map = m_shared.get_ao_image().get();

			m_shared.apply_sky_light(per_frame);

			// Render blended geometry
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
		});
	}

}
