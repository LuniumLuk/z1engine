#pragma once

#include "render/renderer/render_shared.h"
#include "render/renderer/particle_renderer.h"

namespace z1 {

	struct API RendererDeferred {

		RendererDeferred();
		~RendererDeferred();

		void draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer);

		// expose shadow image for other systems
		std::shared_ptr<Image> get_shadow_image() const { return m_shared.m_shadow_image; }

	private:
		void add_gbuffer_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Framebuffer> const& framebuffer, glm::mat4 const& unjittered_projview, uint32_t samples);
		void add_deferred_lighting_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& framebuffer, bool history_uninitialized, int read_idx, std::string const& ao_pass, uint32_t samples);
		void add_gbuffer_resolve_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& framebuffer);
		void add_msaa_edge_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& framebuffer);
		void add_scene_resolve_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& framebuffer);
		void add_ssr_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& framebuffer);
		void add_forward_transparency_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& framebuffer, VisibleDrawList const& draw_list, std::shared_ptr<Scene> const& scene, std::string const& input_pass, std::string const& ao_pass);
		void ensure_lighting_slots();
		void ensure_msaa_slots();

		RenderShared m_shared;
		ParticleRenderer m_particle_renderer;
		std::shared_ptr<MaterialInstance> m_default_material;
		std::shared_ptr<Pipeline> m_pipeline_deferred_lighting;
		// Sample-frequency edge variant of the lighting shader (created on first MSAA frame).
		std::shared_ptr<Pipeline> m_pipeline_deferred_lighting_msaa;
		std::shared_ptr<Pipeline> m_pipeline_ssr;
		std::shared_ptr<Pipeline> m_pipeline_skybox;

		// Fixed sampler slots for the lighting/SSR shaders (resolved on first use).
		struct LightingSlots {
			bool m_valid = false;
			Shader::TextureSlot m_shadow, m_ao, m_sky_ibl;
			Shader::TextureSlot m_gbuffer_position, m_gbuffer_normal, m_gbuffer_albedo, m_gbuffer_mr, m_gbuffer_emissive;
			Shader::TextureSlot m_ssr_scene, m_ssr_position, m_ssr_normal, m_ssr_albedo, m_ssr_mr, m_ssr_depth;
		};
		LightingSlots m_lighting_slots;

		// Slot table for the MSAA edge program (resolved from its own program: units are per program).
		struct MsaaSlots {
			bool m_valid = false;
			Shader::TextureSlot m_shadow, m_ao, m_sky_ibl;
			Shader::TextureSlot m_gbuffer_position, m_gbuffer_normal, m_gbuffer_albedo, m_gbuffer_mr, m_gbuffer_emissive, m_gbuffer_depth;
		};
		MsaaSlots m_msaa_slots;

	};

}
