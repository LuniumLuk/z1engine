#include "render/image.h"
#include "render/buffer.h"
#include "render/vertex_array.h"
#include "render/render_pass.h"
#include "render/framebuffer.h"

namespace z1 {

	struct API RendererForward {

		RendererForward();
		~RendererForward();

		void draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer);

	private:
		void prepare_draw(std::shared_ptr<Framebuffer> const& framebuffer) const;
		void after_draw() const;

		std::shared_ptr<RenderPass> m_render_pass;

	};

}
