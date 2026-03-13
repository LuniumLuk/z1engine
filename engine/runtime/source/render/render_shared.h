#pragma once

#include "render/buffer.h"
#include "render/render_pass.h"
#include "render/framebuffer.h"
#include "render/pipeline.h"
#include "render/vertex_array.h"
#include "render/render_utils.h"
#include "scene/component/sky_light.h"
#include <array>

#define CSM_LAYERS 4

namespace z1 {

	struct Scene;
	struct RenderGraph;
	struct CameraComponent;

	// Light UBO layout (shared between forward and deferred)

	struct alignas(16) LightData {
		glm::vec4 position;  // w = type (0:dir, 1:point, 2:spot)
		glm::vec4 direction; // w = range
		glm::vec4 color;     // w = intensity
		glm::vec4 cone;      // x = inner, y = outer, z = cast_shadow, w = unused
	};

	static const int MAX_LIGHTS = 16;

	struct alignas(16) LightsBlock {
		glm::vec4 count; // x = count
		LightData lights[MAX_LIGHTS];
	};

	// Shared render resources and pass helpers

	struct API RenderShared {

		RenderShared();
		~RenderShared();

		// Shared resources (public so renderers can read them)

		std::shared_ptr<VertexArray> m_quad;
		std::shared_ptr<Pipeline> m_pipeline_postprocess;
		std::shared_ptr<Pipeline> m_pipeline_velocity;
		std::shared_ptr<Pipeline> m_pipeline_taa;
		std::shared_ptr<Pipeline> m_pipeline_bloom_downsample;
		std::shared_ptr<Pipeline> m_pipeline_bloom_upsample;
		std::shared_ptr<Pipeline> m_pipeline_shadow;
		std::shared_ptr<Framebuffer> m_shadow_framebuffer;
		std::shared_ptr<Image> m_shadow_image;
		std::array<std::shared_ptr<Framebuffer>, 2> m_history_colors;
		std::vector<std::shared_ptr<Framebuffer>> m_bloom_textures;
		const int BLOOM_MIP_COUNT = 5;
		std::shared_ptr<UniformBuffer> m_lights_buffer;

		int m_frame_index = 0;

		// Per-frame setup (call from renderer's draw())

		// Ensure bloom/history buffers are sized for the given resolution.
		// Returns true if history buffers were just created (uninitialized).
		bool ensure_buffers(uint32_t width, uint32_t height);

		void update_lights(std::shared_ptr<Scene> const& scene);
		void calculate_csm_splits(CameraComponent& camera, glm::vec3 const& sun_dir);

		// Pass helpers (add to a RenderGraph)

		void add_shadow_pass(RenderGraph& rg, std::shared_ptr<Scene> const& scene);
		void add_velocity_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer, glm::mat4 const& projview);
		void add_taa_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& history_write, std::shared_ptr<Framebuffer> const& history_read);
		void add_bloom_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& source);
		void add_postprocess_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& target, std::shared_ptr<Framebuffer> const& source);

	};

}
