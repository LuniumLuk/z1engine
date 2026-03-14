#pragma once

#include "render/pipeline.h"
#include "render/vertex_array.h"
#include "scene/scene.h"
#include "scene/component/particle.h"
#include "render/render_graph.h"

namespace z1 {

	// Per-vertex data for the shared unit quad (4 vertices)
	struct ParticleQuadVertex {
		glm::vec2 quad_offset;	// billboard offset [-1,1]
		glm::vec2 texcoord;		// UV [0,1]
	};

	// Per-instance data (one per alive particle)
	struct ParticleInstanceData {
		glm::vec3 position;
		float size;
		glm::vec4 color;
		float rotation;
	};

	struct API ParticleRenderer {
		// Pipelines for different blend modes
		std::shared_ptr<Pipeline> m_pipeline_alpha;
		std::shared_ptr<Pipeline> m_pipeline_additive;
		std::shared_ptr<Pipeline> m_pipeline_soft;

		// Shared unit quad geometry (created once)
		std::shared_ptr<VertexBuffer> m_quad_vbo;
		std::shared_ptr<IndexBuffer> m_quad_ibo;
		std::shared_ptr<VertexArray> m_quad_vao;

		void init();
		void shutdown();
		void add_particle_pass(RenderGraph& rg, Scene* scene, std::string const& input_pass);

	private:
		std::shared_ptr<Pipeline> select_pipeline(ParticleBlendMode mode);
	};

}
