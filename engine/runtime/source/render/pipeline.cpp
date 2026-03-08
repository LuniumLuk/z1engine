#include "pch.h"

#include "render/pipeline.h"
#include "render/rhi/opengl_pipeline.h"

namespace z1 {

	std::shared_ptr<Pipeline> Pipeline::build(Description const& description) {
		PROFILE_FUNCTION();
		return std::shared_ptr<Pipeline>(new OpenGLPipeline(description));
	}

}
