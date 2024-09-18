#pragma once

#include "render/buffer.h"

namespace z1 {

    enum struct API PrimitiveType {
        Points,
        LineStrip,
        Lines,
        LineStripAdjacency,
        LinesAdjacency,
        TriangleStrip,
        TriangleFan,
        Triangles,
        TriangleStripAdjacency,
        TrianglesAdjacency,
        Patches,
    };

    struct API VertexArray {
        virtual ~VertexArray() = default;

        virtual void bind() const = 0;
        virtual void unbind() const = 0;
        virtual void draw(PrimitiveType type) = 0;
        virtual void draw_instanced(PrimitiveType type, uint32_t num, std::shared_ptr<VertexBuffer> const& instance_buffer = nullptr, uint32_t start = 0, uint32_t divisor = 0) = 0;

        virtual void* get_native_handle() const = 0;
        static std::shared_ptr<VertexArray> create(std::vector<std::shared_ptr<VertexBuffer>> const& vertex_buffers, std::shared_ptr<IndexBuffer> const& index_buffer = nullptr);
    };

}
