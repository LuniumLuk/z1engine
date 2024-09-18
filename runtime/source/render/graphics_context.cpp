#include "pch.h"
#include "render/graphics_context.h"
#include "render/rhi/opengl_context.h"

namespace z1 {

    std::shared_ptr<GraphicsContext> GraphicsContext::create() {
        return std::shared_ptr<GraphicsContext>(new OpenGLContext());
    }

}