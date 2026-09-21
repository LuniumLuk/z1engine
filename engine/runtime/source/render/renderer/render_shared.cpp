#include "pch.h"
#include "asset/material.h"
#include "render/renderer/render_shared.h"
#include "render/global.h"
#include "render/shader.h"
#include "render/uniform_blocks.h"
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
#include "bakery.h"
#include "tinyexr/tinyexr.h"

#include <array>

namespace {

	constexpr float kPi = 3.14159265359f;

	glm::vec3 sample_latlong_direction(float u, float v) {
		float const phi = (u - 0.5f) * 2.0f * kPi;
		float const elevation = (v - 0.5f) * kPi;
		float const cos_elev = std::cos(elevation);
		return glm::vec3(
			cos_elev * std::cos(phi),
			std::sin(elevation),
			cos_elev * std::sin(phi));
	}

	void accumulate_sh_basis(glm::vec3 const& dir, std::array<glm::vec3, 9>& sh, glm::vec3 const& radiance, float solid_angle) {
		float const x = dir.x;
		float const y = dir.y;
		float const z = dir.z;

		float basis[9] = {
			0.282095f,
			0.488603f * y,
			0.488603f * z,
			0.488603f * x,
			1.092548f * x * y,
			1.092548f * y * z,
			0.315392f * (3.0f * z * z - 1.0f),
			1.092548f * x * z,
			0.546274f * (x * x - y * y)
		};

		for (int i = 0; i < 9; ++i) {
			sh[i] += radiance * basis[i] * solid_angle;
		}
	}

	bool load_texture_pixels(std::shared_ptr<z1::Texture2D> const& texture, std::vector<glm::vec3>& out_pixels, int& out_width, int& out_height) {
		using namespace z1;

		if (!texture || !texture->m_meta.guid.is_valid()) {
			return false;
		}

		auto file = g_runtime_context.m_asset_manager->get_file_from_guid(texture->m_meta.guid);
		if (file.empty()) {
			return false;
		}
		file += ".bin";

		bool is_hdr = false;
		if (texture->m_meta.extra["hdr"]) {
			is_hdr = texture->m_meta.extra["hdr"].as<bool>();
		}

		if (is_hdr) {
			float* data = nullptr;
			char const* err = nullptr;
			int width = 0;
			int height = 0;
			int const success = LoadEXR(&data, &width, &height, file.string().c_str(), &err);
			if (success != TINYEXR_SUCCESS || !data) {
				if (err) {
					FreeEXRErrorMessage(err);
				}
				return false;
			}

			out_width = width;
			out_height = height;
			out_pixels.resize((size_t)width * (size_t)height);

			for (int y = 0; y < height; ++y) {
				int const flipped_y = height - 1 - y;
				for (int x = 0; x < width; ++x) {
					size_t src_idx = ((size_t)flipped_y * (size_t)width + (size_t)x) * 4;
					size_t dst_idx = (size_t)y * (size_t)width + (size_t)x;
					out_pixels[dst_idx] = glm::vec3(data[src_idx + 0], data[src_idx + 1], data[src_idx + 2]);
				}
			}

			free(data);
			return true;
		}

		int width = 0;
		int height = 0;
		auto* data = bakery::load_compressed_image(file, &width, &height);
		if (!data || width <= 0 || height <= 0) {
			return false;
		}

		out_width = width;
		out_height = height;
		out_pixels.resize((size_t)width * (size_t)height);
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				size_t idx = ((size_t)y * (size_t)width + (size_t)x) * 4;
				glm::vec3 srgb = glm::vec3(
					(float)data[idx + 0] / 255.0f,
					(float)data[idx + 1] / 255.0f,
					(float)data[idx + 2] / 255.0f);
				// Convert to linear to match lighting space.
				out_pixels[(size_t)y * (size_t)width + (size_t)x] = glm::pow(srgb, glm::vec3(2.2f));
			}
		}

		bakery::free_loaded_data(data);
		return true;
	}

}

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
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>(ENGINE_RESOURCE("shader/postprocessing"));
			m_pipeline_postprocess = Pipeline::build(desc);
		}

		// (Velocity pass now uses per-material shader variants)

		// TAA pipeline
		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>(ENGINE_RESOURCE("shader/taa"));
			m_pipeline_taa = Pipeline::build(desc);
		}

		// TAA sharpen pipeline
		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>(ENGINE_RESOURCE("shader/taa_sharpen"));
			m_pipeline_taa_sharpen = Pipeline::build(desc);
		}

		// Bloom downsample pipeline
		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>(ENGINE_RESOURCE("shader/bloom_downsample"));
			m_pipeline_bloom_downsample = Pipeline::build(desc);
		}

		// Bloom upsample pipeline
		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.blend = true;
			desc.src_blend_factor = BlendFactor::One;
			desc.dst_blend_factor = BlendFactor::One;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>(ENGINE_RESOURCE("shader/bloom_upsample"));
			m_pipeline_bloom_upsample = Pipeline::build(desc);
		}

		// SSAO pipeline
		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>(ENGINE_RESOURCE("shader/ssao"));
			m_pipeline_ssao = Pipeline::build(desc);
		}

		// GTAO pipeline
		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>(ENGINE_RESOURCE("shader/gtao"));
			m_pipeline_gtao = Pipeline::build(desc);
		}

		// AO blur pipeline
		{
			Pipeline::Description desc{};
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>(ENGINE_RESOURCE("shader/ao_blur"));
			m_pipeline_ao_blur = Pipeline::build(desc);
		}

		// Shadow framebuffer (shadow pass now uses per-material shader variants)
		ensure_shadow_resources();

		m_lights_buffer = UniformBuffer::create(nullptr, sizeof(LightsBlock), BufferUsage::Static);

		// Per-instance pool: intermediates stay cached per scene, never shared globally.
		m_framebuffer_pool = std::make_shared<FramebufferPool>();
	}

	RenderShared::~RenderShared() {
	}

	uint32_t RenderShared::get_effective_msaa_samples() {
		uint32_t requested = static_cast<uint32_t>(g_runtime_context.m_global->msaa_samples);

		// Test harness override (same spirit as Z1_AUTOROTATE): forces the sample count, 1 = off.
		if (char const* override_value = std::getenv("Z1_MSAA")) {
			requested = static_cast<uint32_t>(std::max(1, atoi(override_value)));
		}

		if (requested <= 1) {
			return 1;
		}

		uint32_t const max_supported = g_runtime_context.m_graphics_context->get_max_msaa_samples();
		uint32_t effective = std::min(requested, max_supported);
		if (effective < 2) {
			if (m_msaa_warned_request != requested) {
				CORE_WARN("MSAA {0}x requested but this driver supports at most {1}x; MSAA disabled", requested, max_supported);
				m_msaa_warned_request = requested;
			}
			return 1;
		}
		if (effective != requested && m_msaa_warned_request != requested) {
			CORE_WARN("MSAA {0}x requested but this driver supports at most {1}x; using {1}x", requested, max_supported);
			m_msaa_warned_request = requested;
		}
		return effective;
	}

	void RenderShared::draw_msaa_depth_resolve(RenderGraphNode& node, GraphicsContext& ctx, std::string const& depth_input) {
		if (!m_pipeline_msaa_depth_resolve) {
			Pipeline::Description desc{};
			desc.depth_test = true;
			desc.depth_write = true;
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>(ENGINE_RESOURCE("shader/msaa_depth_resolve"));
			m_pipeline_msaa_depth_resolve = Pipeline::build(desc);
			m_msaa_depth_slot = m_pipeline_msaa_depth_resolve->m_shader->sampler_slot("u_depth_ms");
		}

		m_pipeline_msaa_depth_resolve->bind();
		auto& s = m_pipeline_msaa_depth_resolve->m_shader;
		node.bind_input(s, m_msaa_depth_slot, depth_input);

		// gl_NumSamples would report the single-sample draw target; use the source texture's count.
		auto image = node.get_input_image_name(depth_input);
		int const num_samples = image ? static_cast<int>(image->get_description().m_samples) : 1;
		s->set_uniform("u_num_samples", &num_samples);

		m_quad->bind();
		m_quad->draw(PrimitiveType::Triangles);
		m_quad->unbind();

		m_pipeline_msaa_depth_resolve->unbind();
	}

	bool RenderShared::ensure_buffers(uint32_t width, uint32_t height) {
		// shadow resources follow the shadow settings, not the viewport size
		ensure_shadow_resources();

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

		// AO buffers (half or quarter resolution, selected by the global setting)
		{
			auto& g = g_runtime_context.m_global;
			uint32_t const divisor = std::max(2u, (uint32_t)g->ao_resolution);
			uint32_t ao_width = std::max(1u, width / divisor);
			uint32_t ao_height = std::max(1u, height / divisor);

			if (!m_ao_framebuffer ||
				m_ao_framebuffer->get_width() != ao_width ||
				m_ao_framebuffer->get_height() != ao_height) {
				std::vector<Framebuffer::Attachment> ao_attachments = {
					{ ImageFormat::RGBA8, SamplerMode::Linear, WrapMode::ClampToEdge }
				};
				m_ao_framebuffer = Framebuffer::create(ao_width, ao_height, ao_attachments);
				m_ao_blur_framebuffer = Framebuffer::create(ao_width, ao_height, ao_attachments);
			}

			m_ao_texel_size = { 1.0f / (float)ao_width, 1.0f / (float)ao_height };
		}

		return history_uninitialized;
	}

	void RenderShared::ensure_shadow_resources() {
		// defensive: the renderers are constructed before GlobalSettings exists (see
		// RuntimeContext::init), so fall back to the engine defaults the first time; the
		// per-frame ensure_buffers() call then reconciles with the actual settings
		auto& g = g_runtime_context.m_global;
		uint32_t const resolution = g ? (uint32_t)g->sm_resolution : 2048u;
		uint32_t const cascades = g ? std::min(std::max((int)g->sm_cascade_count, 1), MAX_CSM_CASCADES) : 4u;

		if (m_shadow_framebuffer && m_shadow_resolution == resolution && m_shadow_cascade_count == cascades) {
			return;
		}

		Framebuffer::Attachment attachment;
		attachment.format = ImageFormat::Depth;
		attachment.sampler_mode = SamplerMode::Nearest;
		attachment.wrap_mode = WrapMode::ClampToBorder;
		attachment.layers = cascades;
		// the shadow map is always a texture array (sampled as sampler2DArray, one layer per cascade)
		attachment.layered = true;
		m_shadow_framebuffer = Framebuffer::create(resolution, resolution, { attachment });
		m_shadow_image = m_shadow_framebuffer->get_attachment_image(0);
		m_shadow_resolution = resolution;
		m_shadow_cascade_count = cascades;
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

	void RenderShared::update_sky_light(std::shared_ptr<Scene> const& scene) {
		auto& g = g_runtime_context.m_global;
		auto sky_view = scene->m_registry.view<SkyLightComponent const>();
		SkyLightComponent const* active_sky = nullptr;
		for (auto [entity, sky] : sky_view.each()) {
			active_sky = &sky;
			break;
		}

		if (!active_sky || !active_sky->m_texture || !active_sky->m_texture->m_image) {
			m_has_sky_light = false;
			m_sky_ibl_image.reset();
			m_sky_rotation = 0.0f;
			m_sky_intensity = 0.0f;
			m_sky_mip_level = 0.0f;
			m_sky_specular_max_mip = 0.0f;
			m_sky_sh_ready = false;
			m_sky_sh_guid = {};
			g->sky_params = glm::vec4(0.0f);
			for (auto& coeff : m_sky_sh_coeffs) {
				coeff = glm::vec4(0.0f);
			}
			for (int i = 0; i < 9; ++i) {
				g->sky_sh[i] = glm::vec4(0.0f);
			}
			return;
		}

		m_has_sky_light = true;
		m_sky_ibl_image = active_sky->m_texture->m_image;
		m_sky_rotation = active_sky->m_rotation;
		m_sky_intensity = active_sky->m_intensity;
		m_sky_mip_level = active_sky->m_mip_level;

		auto const& desc = m_sky_ibl_image->get_description();
		float const max_dim = (float)std::max(desc.m_width, desc.m_height);
		m_sky_specular_max_mip = std::max(0.0f, std::floor(std::log2(std::max(1.0f, max_dim))));
		g->sky_params = glm::vec4(m_sky_rotation, m_sky_intensity, m_sky_mip_level, m_sky_specular_max_mip);

		Guid const current_guid = active_sky->m_texture->m_meta.guid;
		bool const need_rebuild = !m_sky_sh_ready || (current_guid != m_sky_sh_guid);
		if (!need_rebuild) {
			// Another scene's draw may have zeroed sky_sh; re-push the cached coefficients.
			for (int i = 0; i < 9; ++i) {
				g->sky_sh[i] = m_sky_sh_coeffs[i];
			}
			return;
		}

		std::array<glm::vec3, 9> sh = {};
		std::vector<glm::vec3> pixels;
		int width = 0;
		int height = 0;
		if (load_texture_pixels(active_sky->m_texture, pixels, width, height) && width > 0 && height > 0) {
			float const dphi = (2.0f * kPi) / (float)width;
			float const delev = kPi / (float)height;

			for (int y = 0; y < height; ++y) {
				float const v = ((float)y + 0.5f) / (float)height;
				float const elevation = (v - 0.5f) * kPi;
				float const solid_angle = std::cos(elevation) * dphi * delev;

				for (int x = 0; x < width; ++x) {
					float const u = ((float)x + 0.5f) / (float)width;
					glm::vec3 const dir = sample_latlong_direction(u, v);
					glm::vec3 const radiance = pixels[(size_t)y * (size_t)width + (size_t)x];
					accumulate_sh_basis(dir, sh, radiance, solid_angle);
				}
			}
		}

		for (int i = 0; i < 9; ++i) {
			m_sky_sh_coeffs[i] = glm::vec4(sh[i], 0.0f);
			g->sky_sh[i] = m_sky_sh_coeffs[i];
		}
		m_sky_sh_guid = current_guid;
		m_sky_sh_ready = true;
	}

	void RenderShared::apply_sky_light(PerFrameConst& per_frame) const {
		if (!m_has_sky_light || !m_sky_ibl_image) {
			return;
		}

		per_frame.sky_ibl_map = m_sky_ibl_image.get();
	}

	void RenderShared::calculate_csm_splits(CameraComponent& camera, glm::vec3 const& sun_dir) {
		auto& g = g_runtime_context.m_global;
		int const cascade_count = std::min(std::max((int)g->sm_cascade_count, 1), MAX_CSM_CASCADES);
		float near_clip = camera.m_near;
		float far_clip = camera.m_far;
		float split_lambda = 0.95f;

		float splits[MAX_CSM_CASCADES + 1] = {};
		splits[0] = near_clip;
		splits[cascade_count] = far_clip;

		for (int i = 1; i < cascade_count; i++) {
			float p = (float)i / (float)cascade_count;
			float log = near_clip * std::pow(far_clip / near_clip, p);
			float uniform = near_clip + (far_clip - near_clip) * p;
			splits[i] = split_lambda * log + (1.0f - split_lambda) * uniform;
		}

		// unused split slots repeat the camera far plane; the shader only tests the splits
		// belonging to cascades that exist, so they are never read in practice
		float split_values[MAX_CSM_CASCADES] = {};
		for (int i = 0; i < MAX_CSM_CASCADES; ++i) {
			split_values[i] = (i + 1) <= cascade_count ? splits[i + 1] : far_clip;
		}
		g->csm_splits = glm::vec4(split_values[0], split_values[1], split_values[2], split_values[3]);

		glm::mat4 inv_view = glm::inverse(camera.get_view());
		glm::vec3 cam_pos_world = glm::vec3(inv_view[3]);

		// Cascade 0 half-extent = sm_ortho_size; farther cascades scale with coverage distance.
		float const base_extent = std::max(g->sm_ortho_size, 1.0f);

		for (int i = 0; i < cascade_count; ++i) {
			float cascade_far = splits[i + 1];

			// Rotation-stable center + texel-snapped light-view translation
			glm::vec3 center = cam_pos_world;
			float size = std::min(base_extent * (cascade_far / splits[1]), camera.m_far);

			glm::vec3 light_pos = center + glm::normalize(sun_dir) * size;

			glm::mat4 light_view = glm::lookAt(light_pos, center, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 light_proj = glm::ortho(-size, size, -size, size, -size * 6.0f, size * 6.0f);

			// glm matrices are column-major: the translation lives in column 3, rows 0/1.
			float world_units_per_texel = (size * 2.0f) / m_shadow_framebuffer->get_width();
			light_view[3][0] = std::floor(light_view[3][0] / world_units_per_texel) * world_units_per_texel;
			light_view[3][1] = std::floor(light_view[3][1] / world_units_per_texel) * world_units_per_texel;

			g->sun_projview[i] = light_proj * light_view;
		}

		// unused cascade slots keep a valid matrix so shaders never read uninitialized data
		for (int i = cascade_count; i < MAX_CSM_CASCADES; ++i) {
			g->sun_projview[i] = g->sun_projview[cascade_count - 1];
		}
	}

	void RenderShared::add_shadow_pass(RenderGraph& rg, std::shared_ptr<Scene> const& scene, std::shared_ptr<MaterialInstance> const& default_material) {
		auto& g = g_runtime_context.m_global;
		RenderPass::Description desc;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Clear;
		desc.depth_stencil_attachment.clear_depth_value = 1.0f;

		int const cascade_count = std::min(std::max((int)g->sm_cascade_count, 1), MAX_CSM_CASCADES);

		for (int cascade = 0; cascade < cascade_count; ++cascade) {
			rg.add_pass(std::string("shadow-CSM") + std::to_string(cascade))
				.set_output(m_shadow_framebuffer)
				.set_pass_desc(desc)
				.pre_pass([&, cascade](RenderGraphNode& node, GraphicsContext& ctx) {
					m_shadow_framebuffer->set_attachment_layer(0, cascade);
				})
				.execute([cascade, scene, default_material](RenderGraphNode& node, GraphicsContext& ctx) {

				// -- Static meshes --
				auto view = scene->m_registry.view<TransformComponent const, StaticMeshComponent const>();
				for (auto [entity, transform, mesh] : view.each()) {
					if (!mesh.m_mesh) continue;
					glm::mat4 model = transform.get_world_transform();

					PerFrameConst per_frame{};
					per_frame.model = model;
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
					}
				}

				// -- Skeletal meshes --
				auto view_skel = scene->m_registry.view<TransformComponent const, SkeletalMeshComponent const>();
				for (auto [entity, transform, mesh] : view_skel.each()) {
					if (!mesh.m_mesh) continue;
					glm::mat4 model = transform.get_world_transform();

					PerFrameConst per_frame{};
					per_frame.model = model;
					per_frame.variant_key = ShaderVariant::Shadow;

					int has_skinning = 0;
					if (scene->m_registry.all_of<AnimationComponent>(entity)) {
						auto const& anim = scene->m_registry.get<AnimationComponent>(entity);
						if (anim.bone_ubo) {
							has_skinning = 1;
							ctx.bind_uniform_buffer(uniform_blocks::Bones, *anim.bone_ubo);
						}
					}

					for (auto const& prim : mesh.m_mesh->m_primitives) {
						std::shared_ptr<MaterialInstance> mi = nullptr;
						if (prim.m_material.is_valid()) {
							mi = g_runtime_context.m_asset_manager->get<MaterialInstance>(prim.m_material);
						}
						if (!mi) mi = default_material;

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
							ctx.bind_uniform_buffer(uniform_blocks::Bones, *anim.bone_ubo);
						}

						prim.m_vertex_array->bind();
						prim.m_vertex_array->draw(prim.m_primitive_type);
						prim.m_vertex_array->unbind();

						mi->unbind();
					}
				}
			});
		}
	}

	std::string RenderShared::add_ao_pass(RenderGraph& rg, std::string const& depth_input, std::string const& normal_input) {
		auto& g = g_runtime_context.m_global;
		if (!g->ao_enabled) {
			return "";
		}
		ensure_pass_slots();

		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::Clear;
		desc.color_attachments[0].clear_value = { 1.0f, 1.0f, 1.0f, 1.0f };
		desc.depth_stencil_attachment.depth_load_op = LoadOp::DontCare;

		bool use_gtao = g->ao_type == AOMode::GTAO;

		rg.add_pass("ao")
			.set_output(m_ao_framebuffer)
			.set_pass_desc(desc)
			.add_input(depth_input)
			.add_input(normal_input)
			.execute([this, use_gtao, depth_input, normal_input](RenderGraphNode& node, GraphicsContext& ctx) {
				auto pipeline = use_gtao ? m_pipeline_gtao : m_pipeline_ssao;
				pipeline->bind();
				auto& s = pipeline->m_shader;

				auto& g = g_runtime_context.m_global;
				ctx.bind_uniform_buffer(uniform_blocks::Global, g->get_buffer());
				s->set_uniform("u_proj", &m_proj);
				s->set_uniform("u_inv_proj", &m_inv_proj);
				s->set_uniform("u_view", &m_view);
				s->set_uniform("u_radius", &g->ao_radius);
				s->set_uniform("u_bias", &g->ao_bias);
				s->set_uniform("u_intensity", &g->ao_intensity);
				s->set_uniform("u_power", &g->ao_power);

node.bind_input(s, m_pass_slots.m_ao_depth, depth_input);
			node.bind_input(s, m_pass_slots.m_ao_normal, normal_input);

				m_quad->bind();
				m_quad->draw(PrimitiveType::Triangles);
				m_quad->unbind();

				pipeline->unbind();
			});

		if (!g->ao_blur_enabled) {
			return "ao";
		}

		RenderPass::Description blur_desc;
		blur_desc.color_attachments.resize(1);
		blur_desc.color_attachments[0].load_op = LoadOp::DontCare;
		blur_desc.depth_stencil_attachment.depth_load_op = LoadOp::DontCare;

		rg.add_pass("ao-blur")
			.set_output(m_ao_blur_framebuffer)
			.set_pass_desc(blur_desc)
			.add_input(depth_input)
			.depends_on("ao")
			.execute([this, depth_input](RenderGraphNode& node, GraphicsContext& ctx) {
				m_pipeline_ao_blur->bind();
				auto& s = m_pipeline_ao_blur->m_shader;

				auto& g = g_runtime_context.m_global;
				ctx.bind_uniform_buffer(uniform_blocks::Global, g->get_buffer());
				s->set_uniform("u_proj", &m_proj);
				s->set_uniform("u_texel_size", &m_ao_texel_size);
				s->set_uniform("u_strength", &g->ao_blur_strength);

				auto ao_img = m_ao_framebuffer->get_attachment_image(0);
				s->bind_texture(m_pass_slots.m_ao_blur_ao, ao_img.get());
				node.bind_input(s, m_pass_slots.m_ao_blur_depth, depth_input);

				m_quad->bind();
				m_quad->draw(PrimitiveType::Triangles);
				m_quad->unbind();

				m_pipeline_ao_blur->unbind();
			});

		return "ao-blur";
	}

	std::shared_ptr<Image> RenderShared::get_ao_image() const {
		auto& g = g_runtime_context.m_global;
		if (!g->ao_enabled || !m_ao_framebuffer) {
			return nullptr;
		}
		auto framebuffer = g->ao_blur_enabled ? m_ao_blur_framebuffer : m_ao_framebuffer;
		return framebuffer ? framebuffer->get_attachment_image(0) : nullptr;
	}

	void RenderShared::add_depth_prepass_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Framebuffer> const& framebuffer, std::shared_ptr<MaterialInstance> const& default_material) {
		RenderPass::Description desc;
		desc.color_attachments.resize(2);
		desc.color_attachments[0].load_op = LoadOp::Clear;
		desc.color_attachments[0].clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };
		desc.color_attachments[1].load_op = LoadOp::Clear;
		desc.color_attachments[1].clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };
		desc.depth_stencil_attachment.depth_load_op = LoadOp::Clear;
		desc.depth_stencil_attachment.clear_depth_value = 1.0f;

		// Renders the GBuffer variant into a reduced framebuffer (position + normal +
		// depth). The shader's extra MRT outputs (albedo/MR/emissive) are dropped by
		// OpenGL because no matching attachments exist.
		rg.add_pass("prepass")
			.set_resolution_as(framebuffer)
			.set_pass_desc(desc)
			.add_output("prepass-position", ImageFormat::RGBA32F, SamplerMode::Nearest, WrapMode::ClampToEdge)
			.add_output("prepass-normal", ImageFormat::RGB16F, SamplerMode::Nearest, WrapMode::ClampToEdge)
			.add_output("prepass-depth", ImageFormat::Depth, SamplerMode::Nearest, WrapMode::ClampToEdge)
			.execute([this, &draw_list, default_material](RenderGraphNode& node, GraphicsContext& ctx) {
				PerFrameConst per_frame{};
				per_frame.variant_key = ShaderVariant::GBuffer;
				per_frame.lights = m_lights_buffer.get();

				// Render opaque and masked geometry only (blend surfaces come later)
				for (auto const& item : draw_list.static_meshes) {
					per_frame.model = item.transform;
					auto* overrides = item.mesh->override_materials_or_null();
					item.mesh->m_mesh->draw(per_frame, default_material, overrides, [](uint32_t flags) {
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
					item.mesh->m_mesh->draw(per_frame, default_material, bones, overrides, [](uint32_t flags) {
						return MaterialFlags::get_alpha_mode(flags) != AlphaMode::Blend;
					});
				}
			});
	}

	void RenderShared::add_velocity_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer, glm::mat4 const& projview, std::shared_ptr<MaterialInstance> const& default_material) {
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
			.execute([&draw_list, projview, default_material](RenderGraphNode& node, GraphicsContext& ctx) {
				auto& g = g_runtime_context.m_global;

				auto jittered_projview = g->projview;
				g->projview = projview;
				g->flush();

				// -- Static meshes --
				for (auto const& item : draw_list.static_meshes) {
					PerFrameConst per_frame{};
					per_frame.model = item.transform;
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
					per_frame.variant_key = ShaderVariant::Velocity;

					int has_skinning = 0;
					int use_prev_bones = 0;

					if (item.anim && item.anim->bone_ubo) {
						has_skinning = 1;
						ctx.bind_uniform_buffer(uniform_blocks::Bones, *item.anim->bone_ubo);
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
							ctx.bind_uniform_buffer(uniform_blocks::Bones, *item.anim->bone_ubo);
							if (g->anim_enabled && g->taa_animated && item.anim->prev_bone_ubo) {
								ctx.bind_uniform_buffer(uniform_blocks::PrevBones, *item.anim->prev_bone_ubo);
								use_prev_bones = 1;
								s->set_uniform("u_use_prev_bones", &use_prev_bones);
							}
						}

						prim.m_vertex_array->bind();
						prim.m_vertex_array->draw(prim.m_primitive_type);
						prim.m_vertex_array->unbind();

						mi->unbind();
					}
				}

				g->projview = jittered_projview;
				g->flush();

				});
	}

	void RenderShared::add_taa_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& history_write, std::shared_ptr<Framebuffer> const& history_read, std::string const& scene_color_input) {
		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::DontCare;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::DontCare;

		rg.add_pass("taa")
			.set_output(history_write)
			.set_pass_desc(desc)
			.add_input(scene_color_input)
			.add_input("velocity")
			.execute([this, history_read, scene_color_input](RenderGraphNode& node, GraphicsContext& ctx) {
				auto h = history_read->get_attachment_image(0);

				m_pipeline_taa->bind();
				auto& s = m_pipeline_taa->m_shader;
				ctx.bind_uniform_buffer(uniform_blocks::Global, g_runtime_context.m_global->get_buffer());
				node.bind_input(s, m_pass_slots.m_taa_current, scene_color_input);
				s->bind_texture(m_pass_slots.m_taa_history, h.get());
				node.bind_input(s, m_pass_slots.m_taa_velocity, "velocity");

				m_quad->bind();
				m_quad->draw(PrimitiveType::Triangles);
				m_quad->unbind();

				m_pipeline_taa->unbind();
				});
	}

	void RenderShared::add_taa_sharpen_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& source) {
		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::DontCare;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::DontCare;

		rg.add_pass("taa-sharpen")
			.set_resolution_as(source)
			.depends_on("taa")
			.add_output("taa-sharpen", ImageFormat::RGBA32F, SamplerMode::Linear, WrapMode::ClampToBorder)
			.set_pass_desc(desc)
			.execute([this, source](RenderGraphNode& node, GraphicsContext& ctx) {
				auto src_img = source->get_attachment_image(0);

				m_pipeline_taa_sharpen->bind();
				auto& s = m_pipeline_taa_sharpen->m_shader;
				ctx.bind_uniform_buffer(uniform_blocks::Global, g_runtime_context.m_global->get_buffer());
				s->bind_texture(m_pass_slots.m_sharpen_src, src_img.get());

				m_quad->bind();
				m_quad->draw(PrimitiveType::Triangles);
				m_quad->unbind();

				m_pipeline_taa_sharpen->unbind();
				});
	}

	void RenderShared::add_bloom_pass(RenderGraph& rg, std::string const& input) {
		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::DontCare;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::DontCare;

		if (m_bloom_textures.empty()) return;

		// Downsample
		for (int i = 0; i < BLOOM_MIP_COUNT; i++) {
			auto target = m_bloom_textures[i];
			std::string name = "bloom-down-" + std::to_string(i);

			auto& pass = rg.add_pass(name);
			pass.set_output(target)
				.set_pass_desc(desc);

			if (i == 0) {
				pass.add_input(input);
			}
			else {
				pass.depends_on("bloom-down-" + std::to_string(i - 1));
			}

			pass.execute([this, i, input](RenderGraphNode& node, GraphicsContext& ctx) {
				m_pipeline_bloom_downsample->bind();
				auto& s = m_pipeline_bloom_downsample->m_shader;
				ctx.bind_uniform_buffer(uniform_blocks::Global, g_runtime_context.m_global->get_buffer());

				std::shared_ptr<Image> src_img = nullptr;
				if (i == 0) {
					src_img = node.get_input_image_name(input);
				}
				else {
					src_img = m_bloom_textures[i - 1]->get_attachment_image(0);
				}

				s->bind_texture(m_pass_slots.m_bloom_down_src, src_img.get());

				s->set_uniform("u_mip_level", &i);

				m_quad->bind();
				m_quad->draw(PrimitiveType::Triangles);
				m_quad->unbind();

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
					ctx.bind_uniform_buffer(uniform_blocks::Global, g_runtime_context.m_global->get_buffer());

					auto src_img = m_bloom_textures[i]->get_attachment_image(0);
					s->bind_texture(m_pass_slots.m_bloom_up_src, src_img.get());

					float radius = 1.0f;
					s->set_uniform("u_filter_radius", &radius);

					m_quad->bind();
					m_quad->draw(PrimitiveType::Triangles);
					m_quad->unbind();

					m_pipeline_bloom_upsample->unbind();
				});
		}
	}

	void RenderShared::add_postprocess_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& target, std::string const& scene_input, bool bloom_present) {
		RenderPass::Description desc;
		desc.color_attachments.resize(1);
		desc.color_attachments[0].load_op = LoadOp::DontCare;
		desc.depth_stencil_attachment.depth_load_op = LoadOp::DontCare;

		auto& pass = rg.add_pass("postprocessing");
		pass.set_output(target)
			.set_pass_desc(desc)
			.add_input(scene_input);
		if (bloom_present) {
			pass.depends_on("bloom-up-1");
		}
		pass.execute([this, bloom_present, scene_input](RenderGraphNode& node, GraphicsContext& ctx) {
				m_pipeline_postprocess->bind();
				auto& s = m_pipeline_postprocess->m_shader;
				ctx.bind_uniform_buffer(uniform_blocks::Global, g_runtime_context.m_global->get_buffer());

				bool const sample_bloom = bloom_present &&
					g_runtime_context.m_global->pp_bloom_enabled && !m_bloom_textures.empty();

				auto scene = node.get_input_image_name(scene_input);
				s->bind_texture(m_pass_slots.m_post_scene, scene.get());

				if (sample_bloom) {
					auto bloom = m_bloom_textures[0]->get_attachment_image(0);
					s->bind_texture(m_pass_slots.m_post_bloom, bloom.get());
				}
				else {
					s->bind_texture(m_pass_slots.m_post_bloom, nullptr);
				}

				m_quad->bind();
				m_quad->draw(PrimitiveType::Triangles);
				m_quad->unbind();

				m_pipeline_postprocess->unbind();
				});
	}

	void RenderShared::ensure_pass_slots() {
		if (m_pass_slots.m_valid) {
			return;
		}
		auto const& ao = m_pipeline_gtao->m_shader;
		m_pass_slots.m_ao_depth = ao->sampler_slot("u_depth_texture");
		m_pass_slots.m_ao_normal = ao->sampler_slot("u_normal_texture");

		auto const& blur = m_pipeline_ao_blur->m_shader;
		m_pass_slots.m_ao_blur_ao = blur->sampler_slot("u_ao_texture");
		m_pass_slots.m_ao_blur_depth = blur->sampler_slot("u_depth_texture");

		auto const& taa = m_pipeline_taa->m_shader;
		m_pass_slots.m_taa_current = taa->sampler_slot("u_current_color");
		m_pass_slots.m_taa_history = taa->sampler_slot("u_history_color");
		m_pass_slots.m_taa_velocity = taa->sampler_slot("u_velocity");

		auto const& sharpen = m_pipeline_taa_sharpen->m_shader;
		m_pass_slots.m_sharpen_src = sharpen->sampler_slot("u_src_texture");

		auto const& bloom_down = m_pipeline_bloom_downsample->m_shader;
		m_pass_slots.m_bloom_down_src = bloom_down->sampler_slot("u_src_texture");
		auto const& bloom_up = m_pipeline_bloom_upsample->m_shader;
		m_pass_slots.m_bloom_up_src = bloom_up->sampler_slot("u_src_texture");

		auto const& post = m_pipeline_postprocess->m_shader;
		m_pass_slots.m_post_scene = post->sampler_slot("u_scene");
		m_pass_slots.m_post_bloom = post->sampler_slot("u_bloom_texture");

		m_pass_slots.m_valid = true;
	}

}
