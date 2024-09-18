#include "pch.h"
#include "render/vertex_array.h"
#include "render/rhi/opengl_vertex_array.h"

namespace z1 {

    std::shared_ptr<VertexArray> VertexArray::create(std::vector<std::shared_ptr<VertexBuffer>> const& vertex_buffer, std::shared_ptr<IndexBuffer> const& index_buffer) {
        PROFILE_FUNCTION();
        return std::shared_ptr<VertexArray>(new OpenGLVertexArray(vertex_buffer, index_buffer));
    }

}
