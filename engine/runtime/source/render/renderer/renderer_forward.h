#include "render/buffer.h"
#include "render/render_pass.h"
#include "render/framebuffer.h"
#include "render/pipeline.h"
#include "asset/material.h"
#include "scene/component/sky_light.h"
#include <array>

namespace z1 {

	struct Scene;
	struct RenderGraph;
	struct CameraComponent;
	struct TransformComponent;
	struct StaticMeshComponent;
	struct SkeletalMeshComponent;
	struct AnimationComponent;

	struct VisibleStaticMesh {
		glm::mat4 transform;
		StaticMeshComponent const* mesh;
	};

	struct VisibleSkeletalMesh {
		glm::mat4 transform;
		SkeletalMeshComponent const* mesh;
		AnimationComponent const* anim;
	};

	struct VisibleDrawList {
		std::vector<VisibleStaticMesh> static_meshes;
		std::vector<VisibleSkeletalMesh> skeletal_meshes;
	};

	struct API RendererForward {

		RendererForward();
		~RendererForward();

		void draw(std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer);

		// expose shadow image for other systems
		std::shared_ptr<Image> get_shadow_image() const { return m_shadow_image; }

	private:
		void update_lights(std::shared_ptr<Scene> const& scene);
		void calculate_csm_splits(CameraComponent& camera, glm::vec3 const& sun_dir);

		void add_shadow_pass(RenderGraph& rg, std::shared_ptr<Scene> const& scene);
		void add_main_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer, bool history_uninitialized, int read_idx, glm::mat4 const& projview);
		void add_velocity_pass(RenderGraph& rg, VisibleDrawList const& draw_list, std::shared_ptr<Scene> const& scene, std::shared_ptr<Framebuffer> const& framebuffer, glm::mat4 const& projview);
		void add_taa_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& history_write, std::shared_ptr<Framebuffer> const& history_read);
		void add_postprocess_pass(RenderGraph& rg, std::shared_ptr<Framebuffer> const& target, std::shared_ptr<Framebuffer> const& source);

		std::shared_ptr<MaterialInstance> m_default_material;
		std::shared_ptr<VertexArray> m_quad;
		std::shared_ptr<Pipeline> m_pipeline_postprocess;
		std::shared_ptr<Pipeline> m_pipeline_velocity;
		std::shared_ptr<Pipeline> m_pipeline_taa;
		// std::shared_ptr<Pipeline> m_pipeline_copy; // Removed copy pipeline
		std::shared_ptr<Pipeline> m_pipeline_shadow;
		std::shared_ptr<Pipeline> m_pipeline_skybox;
		std::shared_ptr<Framebuffer> m_shadow_framebuffer;
		std::shared_ptr<Image> m_shadow_image;
		std::array<std::shared_ptr<Framebuffer>, 2> m_history_colors;

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
		std::shared_ptr<UniformBuffer> m_lights_buffer;

	};

}
