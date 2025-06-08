#pragma once

#include "core/core.h"
#include "scene/component.h"
#include "entt.hpp"

namespace z1 {

    struct API Scene {
        Scene();
        ~Scene();

        void on_update(float delta_time);

        entt::entity create_entity() {
            return m_registry.create();
        }

        entt::registry& get_registry() {
            return m_registry;
        }

    private:
        entt::registry m_registry;
    };

}
