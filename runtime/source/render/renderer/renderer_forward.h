#include "render/global.h"
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

		//void load_or_create_global_settnigs();

	private:
		std::shared_ptr<MaterialInstance> m_default_material;
		GlobalConstants m_global_data;
		std::shared_ptr<UniformBuffer> m_global_buffer;

	};

}
