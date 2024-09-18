#pragma once

#include "render/render_pass.h"
#include <functional>

namespace z1 {

    struct OpenGLRenderPipeline : RenderPipeline {

        OpenGLRenderPipeline(Description const& description);
        ~OpenGLRenderPipeline();

        void bind() override;
        void unbind() override;

    private:
        std::function<void()> m_bind_func;
        std::function<void()> m_unbind_func;
    };

}
