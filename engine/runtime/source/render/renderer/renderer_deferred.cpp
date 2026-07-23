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
#include "glm/gtc/matrix_transform.hpp"

namespace z1 {

	RendererDeferred::RendererDeferred() {
		m_default_material = g_runtime_context.m_asset_manager->get<MaterialInstance>("$engine/material/MI_phone");

		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("$engine/shader/deferred_lighting");
			m_pipeline_deferred_lighting = Pipeline::build(desc);
		}

		{
			Pipeline::Description desc{};
			desc.depth_test = true;
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("$engine/shader/deferred_skybox");
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

		m_shared.update_lights(scene);
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
		m_shared.add_shadow_pass(rg, scene, m_default_material);
		m_particle_renderer.add_particle_shadow_passes(rg, scene.get(), m_shared.m_shadow_framebuffer, CSM_LAYERS);
		add_gbuffer_pass(rg, draw_list, framebuffer, scene, projview);
		add_deferred_lighting_pass(rg, framebuffer, history_uninitialized, read_idx);
		add_forward_transparency_pass(rg, framebuffer, draw_list, scene);
		m_particle_renderer.add_particle_pass(rg, scene.get(), "forward-transparency", m_shared.m_shadow_image);
		m_shared.add_velocity_pass(rg, draw_list, scene, framebuffer, projview, m_default_material);
		m_shared.add_taa_pass(rg, m_shared.m_history_colors[write_idx], m_shared.m_history_colors[read_idx]);
		m_shared.add_taa_sharpen_pass(rg, m_shared.m_history_colors[write_idx]);
		m_shared.add_bloom_pass(rg);
		m_shared.add_postprocess_pass(rg, framebuffer);

		rg.compile();
		rg.execute();

		++m_shared.m_frame_index;
		g->unbind();
		g->prev_projview = projview;
	}

	// G-buffer pass
	// Renders opaque + masked geometry to a multi-render-target FBO:
	//   RT0: position  (RGB16F)
	//   RT1: normal    (RGB16F)
	//   RT2: albedo    (RGBA8)
	//   RT3: metallic-roughness (RG16F)
	//   DS : depth

	void RendererDeferred::add_gbuffer_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Framebuffer> const& framebuffer, std::shared_ptr<Scene> const& scene, glm::mat4 const& unjittered_projview) {
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
			.add_output("gbuffer-position", ImageFormat::RGB16F, SamplerMode::Nearest, WrapMode::ClampToEdge)
			.add_output("gbuffer-normal", ImageFormat::RGB16F, SamplerMode::Nearest, WrapMode::ClampToEdge)
			.add_output("gbuffer-albedo", ImageFormat::RGBA8, SamplerMode::Nearest, WrapMode::ClampToEdge)
			.add_output("gbuffer-metallic-roughness", ImageFormat::RG16F, SamplerMode::Nearest, WrapMode::ClampToEdge)
			.add_output("gbuffer-emissive", ImageFormat::RGB16F, SamplerMode::Nearest, WrapMode::ClampToEdge)
			.add_output("gbuffer-depth", ImageFormat::Depth)
			.execute([this, &draw_list, scene, unjittered_projview](RenderGraphNode& node, GraphicsContext& ctx) {
				PerFrameConst per_frame{};
				per_frame.global_binding = g_runtime_context.m_global->get_binding();
				per_frame.variant_key = ShaderVariant::GBuffer;

				m_shared.m_lights_buffer->bind();
				per_frame.lights_binding = m_shared.m_lights_buffer->get_binding();

				// Render opaque and masked geometry only
				for (auto const& item : draw_list.static_meshes) {
					per_frame.model = item.transform;
					item.mesh->m_mesh->draw(per_frame, m_default_material, [](uint32_t flags) {
						return MaterialFlags::get_alpha_mode(flags) != AlphaMode::Blend;
					});
				}

				for (auto const& item : draw_list.skeletal_meshes) {
					std::shared_ptr<UniformBuffer> bones = nullptr;
					if (item.anim) {
						bones = item.anim->bone_ubo;
					}

					per_frame.model = item.transform;
					item.mesh->m_mesh->draw(per_frame, m_default_material, bones, [](uint32_t flags) {
						return MaterialFlags::get_alpha_mode(flags) != AlphaMode::Blend;
					});
				}

				// Render skybox to emissive channel of G-buffer for now (could be optimized by skipping depth write and only rendering skybox in deferred lighting pass)
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
						// Use unjittered projview so the skybox is stable across TAA frames
						glm::mat4 inv_projview = glm::inverse(unjittered_projview);
						s->set_uniform("u_inv_projview", &inv_projview);
						s->set_uniform("u_cam_position", &g->cam_position);

						m_shared.m_quad->bind();
						m_shared.m_quad->draw(PrimitiveType::Triangles);
						m_shared.m_quad->unbind();

						sky.m_texture->m_image->unbind();
						m_pipeline_skybox->unbind();
					}
					break;
				}
			});
	}

	// Deferred lighting pass
	// Fullscreen quad that reads G-buffer + shadow map + lights UBO
	// and writes lit scene-color.

	void RendererDeferred::add_deferred_lighting_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& framebuffer, bool history_uninitialized, int read_idx) {
		auto const width = framebuffer->get_width();
		auto const height = framebuffer->get_height();

		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::Clear;
		desc.color_attachments[0].clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Load;

		rg.add_pass("deferred-lighting")
			.set_resolution_as(framebuffer)
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
			})
			.execute([this, history_uninitialized, read_idx, width, height](RenderGraphNode& node, GraphicsContext& ctx) {
				m_pipeline_deferred_lighting->bind();
				auto& s = m_pipeline_deferred_lighting->m_shader;

				auto& g = g_runtime_context.m_global;
				s->set_uniform_block_binding("Global", g->get_binding());

				m_shared.m_lights_buffer->bind();
				s->set_uniform_block_binding("Lights", m_shared.m_lights_buffer->get_binding());

				// Bind shadow map
				m_shared.m_shadow_image->bind();
				s->set_uniform_binding("u_shadow_map", m_shared.m_shadow_image->get_binding());

				// Bind G-buffer textures
				s->set_uniform_binding("u_gbuffer_position", node.bind_input_index(0));
				s->set_uniform_binding("u_gbuffer_normal", node.bind_input_index(1));
				s->set_uniform_binding("u_gbuffer_albedo", node.bind_input_index(2));
				s->set_uniform_binding("u_gbuffer_metallic_roughness", node.bind_input_index(3));
				s->set_uniform_binding("u_gbuffer_emissive", node.bind_input_index(4));

				m_shared.m_quad->bind();
				m_shared.m_quad->draw(PrimitiveType::Triangles);
				m_shared.m_quad->unbind();

				node.unbind_input_index(0);
				node.unbind_input_index(1);
				node.unbind_input_index(2);
				node.unbind_input_index(3);
				m_shared.m_shadow_image->unbind();
				m_shared.m_lights_buffer->unbind();
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

	// Forward transparency pass
	// Blended objects cannot be deferred. Render them on top of the
	// lit scene-color using normal forward shading + skybox.

	void RendererDeferred::add_forward_transparency_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& framebuffer, VisibleDrawList const& draw_list, std::shared_ptr<Scene> const& scene) {
		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::Load;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Load;

		rg.add_pass("forward-transparency")
			.set_resolution_as(framebuffer)
			.set_pass_desc(desc)
			.set_passthrough("deferred-lighting")
			.execute([this, &draw_list, scene](RenderGraphNode& node, GraphicsContext& ctx) {
				PerFrameConst per_frame{};
				per_frame.global_binding = g_runtime_context.m_global->get_binding();

				m_shared.m_lights_buffer->bind();
				per_frame.lights_binding = m_shared.m_lights_buffer->get_binding();

				m_shared.m_shadow_image->bind();
				per_frame.shadow_map_binding = m_shared.m_shadow_image->get_binding();

				// Render blended geometry
				for (auto const& item : draw_list.static_meshes) {
					per_frame.model = item.transform;
					item.mesh->m_mesh->draw(per_frame, m_default_material, [](uint32_t flags) {
						return MaterialFlags::get_alpha_mode(flags) == AlphaMode::Blend;
					});
				}

				for (auto const& item : draw_list.skeletal_meshes) {
					std::shared_ptr<UniformBuffer> bones = nullptr;
					if (item.anim) {
						bones = item.anim->bone_ubo;
					}

					per_frame.model = item.transform;
					item.mesh->m_mesh->draw(per_frame, m_default_material, bones, [](uint32_t flags) {
						return MaterialFlags::get_alpha_mode(flags) == AlphaMode::Blend;
					});
				}

				m_shared.m_lights_buffer->unbind();
				m_shared.m_shadow_image->unbind();
			});
	}

}
