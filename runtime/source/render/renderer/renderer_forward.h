#include "render/buffer.h"
#include "render/render_pass.h"
#include "render/framebuffer.h"
#include "render/pipeline.h"
#include "asset/material.h"

namespace z1 {

	struct Scene;

	struct API RendererForward {

		RendererForward();
		~RendererForward();

		void draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer);

		// expose shadow image for other systems
		std::shared_ptr<Image> get_shadow_image() const { return m_shadow_image; }

	private:
		std::shared_ptr<MaterialInstance> m_default_material;
		std::shared_ptr<VertexArray> m_quad;
		std::shared_ptr<Pipeline> m_pipeline_postprocess;
		std::shared_ptr<Pipeline> m_pipeline_velocity;
		std::shared_ptr<Pipeline> m_pipeline_taa;
		std::shared_ptr<Pipeline> m_pipeline_copy;
		std::shared_ptr<Pipeline> m_pipeline_shadow;
		std::shared_ptr<Framebuffer> m_shadow_framebuffer;
		std::shared_ptr<Image> m_shadow_image;
		std::shared_ptr<Framebuffer> m_history_color;

	};

}
