#include "render/buffer.h"
#include "render/render_pass.h"
#include "render/framebuffer.h"
#include "render/pipeline.h"
#include "asset/material.h"

namespace z1 {

	struct API RendererForward {

		RendererForward();
		~RendererForward();

		void draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer);

		//void load_or_create_global_settings();

	private:
		std::shared_ptr<MaterialInstance> m_default_material;
		std::shared_ptr<VertexArray> m_quad;
		std::shared_ptr<Pipeline> m_pipeline_postprocess;

	};

}
