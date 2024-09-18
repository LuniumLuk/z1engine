#pragma once

#include "core/core.h"
#include "glm/glm.hpp"
#include <stack>

namespace z1 {
    struct Framebuffer;
    struct Shader;

    enum struct API BlendFactor {
        Zero = 0,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
    };

    struct API RenderPipeline {

        struct Description {
            // clear
            bool m_clear_color = false;
            bool m_clear_depth = false;
            glm::vec4 m_clear_color_value = { 0.0f, 0.0f, 0.0f, 0.0f };
            float m_clear_depth_value = 1.0f;

            // blend
            bool m_blend = false;
            BlendFactor m_src_blend_factor = BlendFactor::SrcAlpha;
            BlendFactor m_dst_blend_factor = BlendFactor::OneMinusSrcAlpha;

            // viewport
            bool m_dynamic_viewport = true;
            uint32_t m_viewport_x = 0;
            uint32_t m_viewport_y = 0;
            uint32_t m_viewport_width;
            uint32_t m_viewport_height;

            // scissor
            bool m_dynamic_scissor = true;
            uint32_t m_scissor_x = 0;
            uint32_t m_scissor_y = 0;
            uint32_t m_scissor_width;
            uint32_t m_scissor_height;

            // framebuffer
            std::shared_ptr<Framebuffer> m_framebuffer;

            // shader
            std::shared_ptr<Shader> m_shader;
        };

        static std::shared_ptr<RenderPipeline> build(Description const& description);

        virtual void bind() = 0;
        virtual void unbind() = 0;

    };

    struct API RenderPass {

        virtual void build() = 0;
        virtual void destroy() = 0;
        virtual void draw() = 0;

    };

}
