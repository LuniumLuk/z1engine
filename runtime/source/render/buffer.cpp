#include "pch.h"
#include "render/buffer.h"

#include "render/rhi/opengl_buffer.h"

namespace z1 {

	VertexBuffer::Layout::Layout(std::initializer_list<Element> const& elements) {
		m_elements = elements;
		size_t offset = 0;
		for (auto& element : m_elements) {
			element.m_offset = offset;
			offset += get_data_type_size(element.m_type);
		}
		m_stride = offset;
	}

	std::shared_ptr<VertexBuffer> VertexBuffer::create(void const* data, size_t size, Layout const& layout, BufferUsage usage) {
		PROFILE_FUNCTION();
		return std::shared_ptr<VertexBuffer>(new OpenGLVertexBuffer(data, size, layout, usage));
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::create(uint32_t const* data, size_t size, BufferUsage usage) {
		PROFILE_FUNCTION();
		return std::shared_ptr<IndexBuffer>(new OpenGLIndexBuffer(data, size, usage));
	}

	std::shared_ptr<UniformBuffer> UniformBuffer::create(void const* data, size_t size, BufferUsage usage) {
		PROFILE_FUNCTION();
		return std::shared_ptr<UniformBuffer>(new OpenGLUniformBuffer(data, size, usage));
	}

}
