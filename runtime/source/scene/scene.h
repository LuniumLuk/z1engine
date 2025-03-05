#pragma once

#include "core/core.h"
#include "entt.hpp"

namespace z1 {

    struct API Scene {
        Scene() = default;
        ~Scene() = default;

        entt::registry& registry() { return m_registry; }

        entt::entity create_entity() { return m_registry.create(); }

        void on_update(double delta_time);
        void on_fixed_update();

    private:
        entt::registry m_registry;
    };

}
