#include "pch.h"
#include "render/buffer.h"
#include "render/shader.h"
#include "render/graphics_context.h"
#include "render/rhi/opengl_buffer.h"

namespace z1 {

	void UniformBuffer::bind() const {
		CORE_ASSERT(m_binding == INVALID_BINDING || m_ref_count > 0, "uniform buffer is already bound but ref count is 0!");
		if (m_ref_count == 0) {
			m_binding = g_runtime_context.m_graphics_context->acquire_uniform_buffer_binding();
			bind(m_binding);
		}
		++m_ref_count;
	}

	void UniformBuffer::bind(std::shared_ptr<Shader> const& shader, std::string const& name) const {
		bind();
		shader->set_uniform(name, &m_binding);
	}

	void UniformBuffer::unbind() const {
		CORE_ASSERT(m_binding != INVALID_BINDING || m_ref_count == 0, "uniform buffer is not bound but ref count > 0!");
		--m_ref_count;
		if (m_ref_count == 0) {
			unbind(m_binding);
			g_runtime_context.m_graphics_context->release_uniform_buffer_binding(m_binding);
			m_binding = INVALID_BINDING;
		}
	}

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
