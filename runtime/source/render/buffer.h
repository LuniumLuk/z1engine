#pragma once

#include "core/core.h"
#include "render/resource.h"
#include "render/data_types.h"
#include <vector>

namespace z1 {

    enum struct API BufferUsage {
        Static = 0,
        Dynamic,
        Stream,
    };

    struct API VertexBuffer {
        struct Element {
            DataType m_type = DataType::None;
            size_t m_offset = 0;
            bool m_normalized = false;

            Element(DataType type, bool normalized = false)
                : m_type(type), m_offset(0), m_normalized(normalized) {}
        };

        struct Layout {
            std::vector<Element> m_elements;
            size_t m_stride;

            Layout(std::initializer_list<Element> const& elements);
        };

        virtual ~VertexBuffer() = default;

        virtual void bind() const = 0;
        virtual void unbind() const = 0;
        virtual void write(void* data, size_t size = WHOLE_SIZE, size_t offset = 0) = 0;

        virtual void* get_native_handle() const = 0;
        size_t get_size() const { return m_size; }

        static std::shared_ptr<VertexBuffer> create(void* data, size_t size, Layout const& layout, BufferUsage usage = BufferUsage::Static);

    protected:
        size_t m_size = 0;
    };

    struct API IndexBuffer {
        virtual ~IndexBuffer() = default;

        virtual void bind() const = 0;
        virtual void unbind() const = 0;
        virtual void write(void* data, size_t size = WHOLE_SIZE, size_t offset = 0) = 0;

        virtual void* get_native_handle() const = 0;
        size_t get_size() const { return m_size; }

        static std::shared_ptr<IndexBuffer> create(void* data, size_t size, BufferUsage usage = BufferUsage::Static);

    protected:
        size_t m_size = 0;
    };

    struct API UniformBuffer : Resource {
        UniformBuffer() : Resource(ResourceType::UniformBuffer) {}

        virtual ~UniformBuffer() = default;

        virtual void bind() const = 0;
        virtual void unbind() const = 0;
        virtual void write(void* data, size_t size = WHOLE_SIZE, size_t offset = 0) = 0;

        virtual void* get_native_handle() const = 0;
        size_t get_size() const { return m_size; }

        static std::shared_ptr<UniformBuffer> create(void* data, size_t size, BufferUsage usage = BufferUsage::Static);

    protected:
        size_t m_size = 0;
    };

}
