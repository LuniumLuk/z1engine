#pragma once

#include "render/buffer.h"
#include "render/render_pass.h"
#include "render/framebuffer.h"
#include "render/pipeline.h"
#include "render/vertex_array.h"
#include "render/render_utils.h"
#include "render/shader_variant.h"
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

	struct MaterialInstance;

	// Shared render resources and pass helpers

	struct API RenderShared {

		RenderShared();
		~RenderShared();

		// Shared resources (public so renderers can read them)

		std::shared_ptr<VertexArray> m_quad;
		std::shared_ptr<Pipeline> m_pipeline_postprocess;
		std::shared_ptr<Pipeline> m_pipeline_taa;
		std::shared_ptr<Pipeline> m_pipeline_taa_sharpen;
		std::shared_ptr<Pipeline> m_pipeline_bloom_downsample;
		std::shared_ptr<Pipeline> m_pipeline_bloom_upsample;
		// Ambient occlusion
		std::shared_ptr<Pipeline> m_pipeline_ssao;
		std::shared_ptr<Pipeline> m_pipeline_gtao;
		std::shared_ptr<Pipeline> m_pipeline_ao_blur;
		std::shared_ptr<Framebuffer> m_shadow_framebuffer;
		std::shared_ptr<Image> m_shadow_image;
		std::shared_ptr<Framebuffer> m_ao_framebuffer;
		std::shared_ptr<Framebuffer> m_ao_blur_framebuffer;
		glm::vec2 m_ao_texel_size = { 0.0f, 0.0f };
		std::array<std::shared_ptr<Framebuffer>, 2> m_history_colors;
		std::vector<std::shared_ptr<Framebuffer>> m_bloom_textures;
		const int BLOOM_MIP_COUNT = 5;
		std::shared_ptr<UniformBuffer> m_lights_buffer;

		// Per-frame camera matrices used by the AO passes (set by renderers each frame)
		glm::mat4 m_proj = {};
		glm::mat4 m_inv_proj = {};
		glm::mat4 m_view = {};

		int m_frame_index = 0;

		// Per-frame setup (call from renderer's draw())

		// Ensure bloom/history buffers are sized for the given resolution.
		// Returns true if history buffers were just created (uninitialized).
		bool ensure_buffers(uint32_t width, uint32_t height);

		void update_lights(std::shared_ptr<Scene> const& scene);
		void calculate_csm_splits(CameraComponent& camera, glm::vec3 const& sun_dir);

		// Ambient occlusion

		// Adds the AO (+ optional blur) passes reading the given depth/normal graph
		// resources. Returns the final AO pass name ("ao" or "ao-blur"), or "" when
		// AO is disabled. Callers must depends_on() the returned name before sampling
		// get_ao_image().
		std::string add_ao_pass(RenderGraph& rg, std::string const& depth_input, std::string const& normal_input);

		// Returns the AO image to sample in lighting (blurred when enabled, raw
		// otherwise), or nullptr when AO is disabled.
		std::shared_ptr<Image> get_ao_image() const;

		// Forward-pipeline depth+normal prepass (feeds the AO pass before the main
		// lighting pass). Renders opaque + masked geometry using the GBuffer variant.
		void add_depth_prepass_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Framebuffer> const& framebuffer, std::shared_ptr<MaterialInstance> const& default_material);

		// Pass helpers (add to a RenderGraph)

		void add_shadow_pass(RenderGraph& rg, std::shared_ptr<Scene> const& scene, std::shared_ptr<MaterialInstance> const& default_material);
		void add_velocity_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer, glm::mat4 const& projview, std::shared_ptr<MaterialInstance> const& default_material);
		void add_taa_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& history_write, std::shared_ptr<Framebuffer> const& history_read);
		void add_taa_sharpen_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& source);
		void add_bloom_pass(RenderGraph& rg);
		void add_postprocess_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& target);

	};

}
