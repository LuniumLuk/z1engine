#include "pch.h"
#include "render/rhi/opengl_pipeline.h"
#include "glad/glad.h"

namespace z1 {

    static GLenum blend_factor_to_opengl_type(BlendFactor factor) {
        PROFILE_FUNCTION();
        switch (factor) {
        case BlendFactor::Zero: return GL_ZERO;
        case BlendFactor::One: return GL_ONE;
        case BlendFactor::SrcColor: return GL_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DstColor: return GL_DST_COLOR;
        case BlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
        case BlendFactor::SrcAlpha: return GL_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstAlpha: return GL_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
        }
        return GL_ZERO;
    }

    OpenGLRenderPipeline::OpenGLRenderPipeline(Description const& description) {
        m_bind_func = [=]() {
            glClearColor(
                description.m_clear_color_value.r,
                description.m_clear_color_value.g,
                description.m_clear_color_value.b,
                description.m_clear_color_value.a);
            glClearDepth(description.m_clear_depth_value);

            uint32_t clear_flag = 0;
            if (description.m_clear_color) {
                clear_flag |= GL_COLOR_BUFFER_BIT;
            }
            if (description.m_clear_depth) {
                clear_flag |= GL_DEPTH_BUFFER_BIT;
            }
            if (clear_flag) {
                glClear(clear_flag);
            }

            if (description.m_blend) {
                glEnable(GL_BLEND);
                glBlendFunc(
                    blend_factor_to_opengl_type(description.m_src_blend_factor),
                    blend_factor_to_opengl_type(description.m_dst_blend_factor));
            }
            else {
                glDisable(GL_BLEND);
            }

            if (!description.m_dynamic_viewport) {
                glViewport(
                    description.m_viewport_x,
                    description.m_viewport_y,
                    description.m_viewport_width,
                    description.m_viewport_height);
            }

            if (!description.m_dynamic_scissor) {
                glScissor(
                    description.m_scissor_x,
                    description.m_scissor_y,
                    description.m_scissor_width,
                    description.m_scissor_height);
            }
        };

        m_unbind_func = [=]() {};
    }

    OpenGLRenderPipeline::~OpenGLRenderPipeline() {

    }

    void OpenGLRenderPipeline::bind() {
        m_bind_func();
    }

    void OpenGLRenderPipeline::unbind() {
        m_unbind_func();
    }

}
