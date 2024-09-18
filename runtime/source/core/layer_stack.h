#pragma once

#include "core/core.h"
#include "core/layer.h"
#include <vector>

namespace z1 {

    struct API LayerStack {
        LayerStack();
        ~LayerStack();

        void push_layer(std::shared_ptr<Layer> const& layer);
        void push_overlay(std::shared_ptr<Layer> const& overlay);
        void pop_layer(std::shared_ptr<Layer> const& layer);
        void pop_overlay(std::shared_ptr<Layer> const& overlay);

        std::vector<std::shared_ptr<Layer>>::iterator begin() { return m_layers.begin(); }
        std::vector<std::shared_ptr<Layer>>::iterator end() { return m_layers.end(); }

    private:

        /*
        * layer stack:
        *  | layer1 | ... | layerN | overlay1 | ... | overlayM |
        * layer_insert --------^
        *                          overlay_insert --------^
        */

        std::vector<std::shared_ptr<Layer>> m_layers;
        size_t m_layer_insert_index;
    };

}
