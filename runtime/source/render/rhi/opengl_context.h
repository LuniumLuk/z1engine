#pragma once

#include "render/graphics_context.h"

struct GLFWwindow;

namespace z1 {

    struct OpenGLContext : GraphicsContext {
        OpenGLContext();

        void init() override;
        void begin_frame() override {}
        void end_frame() override {}
        void swap_buffers() override;
        void finish() override {}

    private:
        GLFWwindow*  m_window;
    };

}
