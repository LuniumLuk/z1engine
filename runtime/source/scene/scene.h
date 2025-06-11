#pragma once

#include "core/core.h"
#include "scene/component.h"
#include "entt.hpp"

namespace z1 {

    struct Entity;

    struct API Scene : std::enable_shared_from_this<Scene> {
        Scene();
        ~Scene();

        void on_update(float delta_time);

        std::shared_ptr<Entity> create_entity(std::string const& name);

    private:
        friend struct Entity;
        entt::registry m_registry;
    };

}
