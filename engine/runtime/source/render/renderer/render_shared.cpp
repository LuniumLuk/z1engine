#include "pch.h"
#include "asset/material.h"
#include "render/renderer/render_shared.h"
#include "render/global.h"
#include "render/shader.h"
#include "render/framebuffer.h"
#include "render/render_graph.h"
#include "render/graphics_context.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "scene/component/camera.h"
#include "scene/component/mesh.h"
#include "scene/component/light.h"
#include "scene/component/animation.h"
#include "asset/asset_manager.h"
#include "glm/gtc/matrix_transform.hpp"

namespace z1 {

	RenderShared::RenderShared() {
		// Fullscreen quad VAO
		std::vector<float> vertices = {
			-1.0f, -1.0f,
			 1.0f, -1.0f,
			 1.0f,  1.0f,
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

		// Post-process pipeline
		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/postprocessing");
			m_pipeline_postprocess = Pipeline::build(desc);
		}

		// (Velocity pass now uses per-material shader variants)

		// TAA pipeline
		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/taa");
			m_pipeline_taa = Pipeline::build(desc);
		}

		// Bloom downsample pipeline
		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/bloom_downsample");
			m_pipeline_bloom_downsample = Pipeline::build(desc);
		}

		// Bloom upsample pipeline
		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.blend = true;
			desc.src_blend_factor = BlendFactor::One;
			desc.dst_blend_factor = BlendFactor::One;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("shader/bloom_upsample");
			m_pipeline_bloom_upsample = Pipeline::build(desc);
		}

		// Shadow framebuffer (shadow pass now uses per-material shader variants)
		{
			const uint32_t shadow_res = 2048;
			Framebuffer::Attachment attachment;
			attachment.format = ImageFormat::Depth;
			attachment.sampler_mode = SamplerMode::Nearest;
			attachment.wrap_mode = WrapMode::ClampToBorder;
			attachment.layers = 4;
			m_shadow_framebuffer = Framebuffer::create(shadow_res, shadow_res, { attachment });
			m_shadow_image = m_shadow_framebuffer->get_attachment_image(0);
		}

		m_lights_buffer = UniformBuffer::create(nullptr, sizeof(LightsBlock), BufferUsage::Static);
	}

	RenderShared::~RenderShared() {
	}

	bool RenderShared::ensure_buffers(uint32_t width, uint32_t height) {
		// Bloom textures
		if (m_bloom_textures.empty() ||
			m_bloom_textures[0]->get_width() != width / 2 ||
			m_bloom_textures[0]->get_height() != height / 2)
		{
			m_bloom_textures.clear();
			for (int i = 0; i < BLOOM_MIP_COUNT; i++) {
				uint32_t mip_width = width >> (i + 1);
				uint32_t mip_height = height >> (i + 1);
				std::vector<Framebuffer::Attachment> attachments = {
					{ ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToEdge }
				};
				m_bloom_textures.push_back(Framebuffer::create(mip_width, mip_height, attachments));
			}
		}

		// History buffers for TAA
		bool history_uninitialized = false;

		if (!m_history_colors[0]) {
			m_history_colors[0] = Framebuffer::create(
				width, height,
				{ { ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder } });
			m_history_colors[1] = Framebuffer::create(
				width, height,
				{ { ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder } });
			history_uninitialized = true;
		}

		if (m_history_colors[0]->get_width() != width ||
			m_history_colors[0]->get_height() != height) {
			m_history_colors[0]->resize(width, height);
			m_history_colors[1]->resize(width, height);
			history_uninitialized = true;
		}

		return history_uninitialized;
	}

	void RenderShared::update_lights(std::shared_ptr<Scene> const& scene) {
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

	void RenderShared::calculate_csm_splits(CameraComponent& camera, glm::vec3 const& sun_dir) {
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

	void RenderShared::add_shadow_pass(RenderGraph& rg, std::shared_ptr<Scene> const& scene, std::shared_ptr<MaterialInstance> const& default_material) {
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
				.execute([this, cascade, scene, &g, default_material](RenderGraphNode& node, GraphicsContext& ctx) {
				int csm_candidates = 0;
				int csm_drawn = 0;

				// -- Static meshes --
				auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const>();
				for (auto [entity, transform, mesh] : view.each()) {
					if (!mesh.m_mesh) continue;
					glm::mat4 model = transform.get_world_transform();

					PerFrameConst per_frame{};
					per_frame.model = model;
					per_frame.global_binding = g->get_binding();
					per_frame.variant_key = ShaderVariant::Shadow;

					int has_skinning = 0;

					for (auto const& prim : mesh.m_mesh->m_primitives) {
						std::shared_ptr<MaterialInstance> mi = nullptr;
						if (prim.m_material.is_valid()) {
							mi = g_runtime_context.m_asset_manager->get<MaterialInstance>(prim.m_material);
						}
						// Fall back to the default material so meshes without an explicit
						// material assignment still cast shadows (mirrors GBuffer fallback).
						if (!mi) mi = default_material;

						csm_candidates++;
						if (!mi) continue;

						AlphaMode alpha_mode = MaterialFlags::get_alpha_mode(mi->get_flags());
						if (alpha_mode == AlphaMode::Blend)
							continue;

						mi->bind(per_frame);
						auto const& s = mi->get_pipeline(ShaderVariant::Shadow)->m_shader;

						s->set_uniform("u_csm_index", &cascade);
						s->set_uniform("u_has_skinning", &has_skinning);

						prim.m_vertex_array->bind();
						prim.m_vertex_array->draw(prim.m_primitive_type);
						prim.m_vertex_array->unbind();

						mi->unbind();
						csm_drawn++;
					}
				}

				// -- Skeletal meshes --
				auto view_skel = scene->m_registry.view<TransformComponent const, SkeletalMeshComponent const>();
				for (auto [entity, transform, mesh] : view_skel.each()) {
					if (!mesh.m_mesh) continue;
					glm::mat4 model = transform.get_world_transform();

					PerFrameConst per_frame{};
					per_frame.model = model;
					per_frame.global_binding = g->get_binding();
					per_frame.variant_key = ShaderVariant::Shadow;

					int has_skinning = 0;
					if (scene->m_registry.all_of<AnimationComponent>(entity)) {
						auto const& anim = scene->m_registry.get<AnimationComponent>(entity);
						if (anim.bone_ubo) {
							has_skinning = 1;
							anim.bone_ubo->bind();
						}
					}

					for (auto const& prim : mesh.m_mesh->m_primitives) {
						std::shared_ptr<MaterialInstance> mi = nullptr;
						if (prim.m_material.is_valid()) {
							mi = g_runtime_context.m_asset_manager->get<MaterialInstance>(prim.m_material);
						}
						if (!mi) mi = default_material;

						csm_candidates++;
						if (!mi) continue;

						AlphaMode alpha_mode = MaterialFlags::get_alpha_mode(mi->get_flags());
						if (alpha_mode == AlphaMode::Blend)
							continue;

						mi->bind(per_frame);
						auto const& s = mi->get_pipeline(ShaderVariant::Shadow)->m_shader;

						s->set_uniform("u_csm_index", &cascade);
						s->set_uniform("u_has_skinning", &has_skinning);
						if (has_skinning) {
							auto const& anim = scene->m_registry.get<AnimationComponent>(entity);
							s->set_uniform_block_binding("Bones", anim.bone_ubo->get_binding());
						}

						prim.m_vertex_array->bind();
						prim.m_vertex_array->draw(prim.m_primitive_type);
						prim.m_vertex_array->unbind();

						mi->unbind();
						csm_drawn++;
					}

					if (has_skinning) {
						scene->m_registry.get<AnimationComponent>(entity).bone_ubo->unbind();
					}
				}

				// Diagnostic: print every 60 frames so frame 0 always shows in the smoke test
				if (m_frame_index % 60 == 0) {
					std::cout << "[render.shadow] shadow-CSM" << cascade
					          << " frame=" << m_frame_index
					          << " candidates=" << csm_candidates
					          << " drawn=" << csm_drawn << "\n";
				}
			});
		}
	}

	void RenderShared::add_velocity_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer, glm::mat4 const& projview, std::shared_ptr<MaterialInstance> const& default_material) {
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
			.execute([this, &draw_list, projview, &g, default_material](RenderGraphNode& node, GraphicsContext& ctx) {

				auto jittered_projview = g->projview;
				g->projview = projview;
				g->flush();

				// -- Static meshes --
				for (auto const& item : draw_list.static_meshes) {
					PerFrameConst per_frame{};
					per_frame.model = item.transform;
					per_frame.global_binding = g->get_binding();
					per_frame.variant_key = ShaderVariant::Velocity;

					int has_skinning = 0;
					int use_prev_bones = 0;

					for (auto const& prim : item.mesh->m_mesh->m_primitives) {
						std::shared_ptr<MaterialInstance> mi = nullptr;
						if (prim.m_material.is_valid()) {
							mi = g_runtime_context.m_asset_manager->get<MaterialInstance>(prim.m_material);
						}
						if (!mi) mi = default_material;
						if (!mi) continue;

						mi->bind(per_frame);
						auto const& s = mi->get_pipeline(ShaderVariant::Velocity)->m_shader;

						// Set velocity-pass-specific uniforms
						s->set_uniform("u_has_skinning", &has_skinning);
						s->set_uniform("u_use_prev_bones", &use_prev_bones);
						s->set_uniform("u_prev_model", &item.prev_transform);

						prim.m_vertex_array->bind();
						prim.m_vertex_array->draw(prim.m_primitive_type);
						prim.m_vertex_array->unbind();

						mi->unbind();
					}
				}

				// -- Skeletal meshes --
				for (auto const& item : draw_list.skeletal_meshes) {
					PerFrameConst per_frame{};
					per_frame.model = item.transform;
					per_frame.global_binding = g->get_binding();
					per_frame.variant_key = ShaderVariant::Velocity;

					int has_skinning = 0;
					int use_prev_bones = 0;

					if (item.anim && item.anim->bone_ubo) {
						has_skinning = 1;
						item.anim->bone_ubo->bind();
					}

					for (auto const& prim : item.mesh->m_mesh->m_primitives) {
						std::shared_ptr<MaterialInstance> mi = nullptr;
						if (prim.m_material.is_valid()) {
							mi = g_runtime_context.m_asset_manager->get<MaterialInstance>(prim.m_material);
						}
						if (!mi) mi = default_material;
						if (!mi) continue;

						mi->bind(per_frame);
						auto const& s = mi->get_pipeline(ShaderVariant::Velocity)->m_shader;

						s->set_uniform("u_has_skinning", &has_skinning);
						s->set_uniform("u_use_prev_bones", &use_prev_bones);
						s->set_uniform("u_prev_model", &item.prev_transform);

						if (has_skinning) {
							s->set_uniform_block_binding("Bones", item.anim->bone_ubo->get_binding());
							if (g->anim_enabled && g->taa_animated && item.anim->prev_bone_ubo) {
								item.anim->prev_bone_ubo->bind();
								s->set_uniform_block_binding("PrevBones", item.anim->prev_bone_ubo->get_binding());
								use_prev_bones = 1;
								s->set_uniform("u_use_prev_bones", &use_prev_bones);
							}
						}

						prim.m_vertex_array->bind();
						prim.m_vertex_array->draw(prim.m_primitive_type);
						prim.m_vertex_array->unbind();

						mi->unbind();
					}

					if (has_skinning) {
						item.anim->bone_ubo->unbind();
						if (use_prev_bones)
							item.anim->prev_bone_ubo->unbind();
					}
				}

				g->projview = jittered_projview;
				g->flush();

				});
	}

	void RenderShared::add_taa_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& history_write, std::shared_ptr<Framebuffer> const& history_read) {
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

	void RenderShared::add_bloom_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& source) {
		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::DontCare;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::DontCare;

		auto& g = g_runtime_context.m_global;
		if (m_bloom_textures.empty()) return;

		// Downsample
		for (int i = 0; i < BLOOM_MIP_COUNT; i++) {
			auto target = m_bloom_textures[i];
			std::string name = "bloom-down-" + std::to_string(i);

			auto& pass = rg.add_pass(name);
			pass.set_output(target)
				.set_pass_desc(desc);

			if (i == 0) {
				pass.depends_on("taa");
			}
			else {
				pass.depends_on("bloom-down-" + std::to_string(i - 1));
			}

			pass.execute([this, i, source](RenderGraphNode& node, GraphicsContext& ctx) {
				m_pipeline_bloom_downsample->bind();
				auto& s = m_pipeline_bloom_downsample->m_shader;
				s->set_uniform_block_binding("Global", g_runtime_context.m_global->get_binding());

				std::shared_ptr<Image> src_img = nullptr;
				if (i == 0) {
					src_img = source->get_attachment_image(0);
				}
				else {
					src_img = m_bloom_textures[i - 1]->get_attachment_image(0);
				}

				src_img->bind();
				s->set_uniform_binding("u_src_texture", src_img->get_binding());

				s->set_uniform("u_mip_level", &i);

				m_quad->bind();
				m_quad->draw(PrimitiveType::Triangles);
				m_quad->unbind();

				src_img->unbind();
				m_pipeline_bloom_downsample->unbind();
			});
		}

		// Upsample
		for (int i = BLOOM_MIP_COUNT - 1; i > 0; i--) {
			auto target = m_bloom_textures[i - 1];
			std::string name = "bloom-up-" + std::to_string(i);

			rg.add_pass(name)
				.set_output(target)
				.set_pass_desc(desc)
				.depends_on("bloom-down-" + std::to_string(BLOOM_MIP_COUNT - 1))
				.execute([this, i](RenderGraphNode& node, GraphicsContext& ctx) {
					m_pipeline_bloom_upsample->bind();
					auto& s = m_pipeline_bloom_upsample->m_shader;
					s->set_uniform_block_binding("Global", g_runtime_context.m_global->get_binding());

					auto src_img = m_bloom_textures[i]->get_attachment_image(0);
					src_img->bind();
					s->set_uniform_binding("u_src_texture", src_img->get_binding());

					float radius = 1.0f;
					s->set_uniform("u_filter_radius", &radius);

					m_quad->bind();
					m_quad->draw(PrimitiveType::Triangles);
					m_quad->unbind();

					src_img->unbind();
					m_pipeline_bloom_upsample->unbind();
				});
		}
	}

	void RenderShared::add_postprocess_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& target, std::shared_ptr<Framebuffer> const& source) {
		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::DontCare;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::DontCare;

		rg.add_pass("postprocessing")
			.set_output(target)
			.set_pass_desc(desc)
			.depends_on("taa")
			.depends_on("bloom-up-1")
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

				if (g_runtime_context.m_global->pp_bloom_enabled && !m_bloom_textures.empty()) {
					auto& bloom = m_bloom_textures[0]->get_attachment_image(0);
					bloom->bind();
					s->set_uniform_binding("u_bloom_texture", bloom->get_binding());
				}

				m_quad->bind();
				m_quad->draw(PrimitiveType::Triangles);
				m_quad->unbind();

				scene->unbind();

				if (g_runtime_context.m_global->pp_bloom_enabled && !m_bloom_textures.empty()) {
					m_bloom_textures[0]->get_attachment_image(0)->unbind();
				}

				m_pipeline_postprocess->unbind();
				});
	}

}
