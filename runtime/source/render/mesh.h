#pragma once

#include "render/vertex_array.h"
#include "glm/glm.hpp"

namespace z1 {

    struct API Mesh {

        /*
        * TODO: animation
        */

        Mesh(glm::mat4 const& transform, std::shared_ptr<VertexArray> const& vertexArray, PrimitiveType type)
            : m_transform(transform), m_vertex_array(vertexArray), m_primitive_type(type) {}

        void draw() const {
            m_vertex_array->bind();
            m_vertex_array->draw(m_primitive_type);
            m_vertex_array->unbind();
        }

        void draw_instanced(uint32_t num, std::shared_ptr<VertexBuffer> const& instance_buffer, uint32_t start, uint32_t divisor) const {
            m_vertex_array->bind();
            m_vertex_array->draw_instanced(m_primitive_type, num, instance_buffer, start, divisor);
            m_vertex_array->unbind();
        }

        glm::mat4 m_transform;
        std::shared_ptr<VertexArray> m_vertex_array;
        PrimitiveType m_primitive_type;
    };

}
