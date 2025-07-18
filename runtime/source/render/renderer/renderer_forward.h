#include "render/render_pass.h"
#include "render/framebuffer.h"
#include "render/pipeline.h"

namespace z1 {

	struct API RendererForward {

		RendererForward();
		~RendererForward();

		void draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer);

	private:
		std::shared_ptr<RenderPass> m_render_pass;
		// TODO: temporary, later will be replaced by material system
		std::shared_ptr<Pipeline> m_pipeline;

	};

}
