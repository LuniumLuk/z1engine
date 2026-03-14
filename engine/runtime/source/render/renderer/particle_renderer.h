#pragma once

#include "render/pipeline.h"
#include "scene/scene.h"
#include "scene/component/particle.h"
#include "render/render_graph.h"

namespace z1 {

	struct API ParticleRenderer {
		// Pipelines for different blend modes
		std::shared_ptr<Pipeline> m_pipeline_alpha;
		std::shared_ptr<Pipeline> m_pipeline_additive;
		std::shared_ptr<Pipeline> m_pipeline_soft;

		void init();
		void shutdown();
		void add_particle_pass(RenderGraph& rg, Scene* scene, std::string const& input_pass);

	private:
		std::shared_ptr<Pipeline> select_pipeline(ParticleBlendMode mode);
	};

}
