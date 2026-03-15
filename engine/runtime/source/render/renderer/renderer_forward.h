#pragma once

#include "render/renderer/render_shared.h"

namespace z1 {

	struct API RendererForward {

		RendererForward();
		~RendererForward();

		void draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer);

	private:
		void add_main_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer, bool history_uninitialized, int read_idx, glm::mat4 const& projview);

		RenderShared m_shared;
		std::shared_ptr<MaterialInstance> m_default_material;
		std::shared_ptr<Pipeline> m_pipeline_skybox;

	};

}
