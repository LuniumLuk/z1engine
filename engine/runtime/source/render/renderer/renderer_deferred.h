#pragma once

#include "render/render_shared.h"

namespace z1 {

	struct API RendererDeferred {

		RendererDeferred();
		~RendererDeferred();

		void draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer);

		// expose shadow image for other systems
		std::shared_ptr<Image> get_shadow_image() const { return m_shared.m_shadow_image; }

	private:
		void add_gbuffer_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Framebuffer> const& framebuffer);
		void add_deferred_lighting_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& framebuffer, bool history_uninitialized, int read_idx);
		void add_forward_transparency_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& framebuffer, VisibleDrawList const& draw_list, std::shared_ptr<Scene> const& scene);

		RenderShared m_shared;
		std::shared_ptr<MaterialInstance> m_default_material;
		std::shared_ptr<Pipeline> m_pipeline_gbuffer;
		std::shared_ptr<Pipeline> m_pipeline_deferred_lighting;
		std::shared_ptr<Pipeline> m_pipeline_skybox;

	};

}
