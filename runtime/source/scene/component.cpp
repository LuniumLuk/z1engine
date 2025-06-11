#include "pch.h"
#include "scene/component.h"

namespace z1 {

    TagComponent::TagComponent(std::string const& tag)
        : m_tag(tag) {
    }

    TransformComponent::TransformComponent(glm::vec3 const& location, glm::vec3 const& rotation, glm::vec3 const& scale)
        : m_location(location), m_rotation(rotation), m_scale(scale) {
    }

    glm::mat4 TransformComponent::get_transform() const {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_location);
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.x), { 1.0f, 0.0f, 0.0f });
        rotation = glm::rotate(rotation, glm::radians(m_rotation.y), { 0.0f, 1.0f, 0.0f });
        rotation = glm::rotate(rotation, glm::radians(m_rotation.z), { 0.0f, 0.0f, 1.0f });
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_scale);
        return translation * rotation * scale;
    }

    SpriteComponent::SpriteComponent(glm::vec4 const& color)
        : m_color(color) {
    }

}
