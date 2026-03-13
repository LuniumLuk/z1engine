#pragma once

#include "render/render_shared.h"

namespace z1 {

	struct API RendererForward {

		RendererForward();
		~RendererForward();

		void draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer);

		// expose shadow image for other systems
		std::shared_ptr<Image> get_shadow_image() const { return m_shared.m_shadow_image; }

	private:
		void add_main_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer, bool history_uninitialized, int read_idx, glm::mat4 const& projview);

		RenderShared m_shared;
		std::shared_ptr<MaterialInstance> m_default_material;
		std::shared_ptr<Pipeline> m_pipeline_skybox;

	};

}
