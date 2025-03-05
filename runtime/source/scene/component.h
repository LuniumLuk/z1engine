#pragma once

#include "core/core.h"
#include "render/image.h"
#include "glm/glm.hpp"

namespace z1 {

    struct TransformComponent {
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
    };

    struct SpriteComponent {
        glm::vec4 color = glm::vec4(1.0f);
        std::shared_ptr<Image2D> image;
    };

}
