#pragma once

#include "render/vertex_array.h"

namespace z1 {

    struct OpenGLVertexArray : VertexArray {
        OpenGLVertexArray(std::vector<std::shared_ptr<VertexBuffer>> const& vertex_buffers, std::shared_ptr<IndexBuffer> const& index_buffer);
        ~OpenGLVertexArray() override;

        void bind() const override;
        void unbind() const override;
        void draw(PrimitiveType type, uint32_t num = NUM_MAX) override;
        void draw_instanced(PrimitiveType type, uint32_t num, std::shared_ptr<VertexBuffer> const& instance_buffer, uint32_t start, uint32_t divisor) override;

        void* get_native_handle() const override { return (void*)(uint64_t)m_handle; }

    private:
        uint32_t m_element_count;
        bool m_has_indices = false;
        uint32_t m_handle = 0;

        std::vector<std::shared_ptr<VertexBuffer>> m_vertex_buffers;
        std::shared_ptr<IndexBuffer> m_index_buffer;
    };

}
