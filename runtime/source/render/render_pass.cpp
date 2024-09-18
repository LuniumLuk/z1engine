#include "pch.h"
#include "render/render_pass.h"
#include "render/rhi/opengl_pipeline.h"

namespace z1 {

    std::shared_ptr<RenderPipeline> RenderPipeline::build(Description const& description) {
        PROFILE_FUNCTION();
        return std::shared_ptr<RenderPipeline>(new OpenGLRenderPipeline(description));
    }

}
