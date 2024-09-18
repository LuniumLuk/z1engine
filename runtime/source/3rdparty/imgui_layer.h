#pragma once

#include "core/layer.h"

namespace z1 {

    struct API ImGuiLayer : Layer {
        ImGuiLayer();
        ~ImGuiLayer();

        void on_attach() override;
        void on_detach() override;

        void begin();
        void end();
    };

}
