#include "pch.h"
#include "render/rhi/opengl_buffer.h"
#include "render/rhi/opengl_context.h"
#include "glad/glad.h"

namespace z1 {

	// helper functions
	// --------------------------------------------------

	static GLenum buffer_usage_to_opengl_type(BufferUsage usage) {
		switch (usage) {
		case BufferUsage::Static: return GL_STATIC_DRAW;
		case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
		case BufferUsage::Stream: return GL_STREAM_DRAW;
		}
		CORE_ASSERT(false, "unknown buffer usage!");
		return 0;
	}

	static GLenum data_type_to_opengl_type(DataType type) {
		switch (type) {
		case DataType::Float: return GL_FLOAT;
		case DataType::Float2: return GL_FLOAT;
		case DataType::Float3: return GL_FLOAT;
		case DataType::Float4: return GL_FLOAT;
		case DataType::Int: return GL_INT;
		case DataType::Int2: return GL_INT;
		case DataType::Int3: return GL_INT;
		case DataType::Int4: return GL_INT;
		case DataType::Mat3: return GL_FLOAT;
		case DataType::Mat4: return GL_FLOAT;
		case DataType::Bool: return GL_BOOL;
		}
		CORE_ASSERT(false, "unknown data type!");
		return 0;
	}

	static uint32_t create_opengl_buffer(GLenum target, void const* data, size_t size, BufferUsage usage) {
		uint32_t handle = 0;
		glGenBuffers(1, &handle);
		CORE_ASSERT(handle, "failed to create opengl buffer!");
		glBindBuffer(target, handle);
		if (size) {
			glBufferData(target, size, data, buffer_usage_to_opengl_type(usage));
		}
		glBindBuffer(target, 0);
		return handle;
	}

	static void write_opengl_buffer(GLenum target, uint32_t handle, void const* data, size_t size, size_t offset, size_t totalSize) {
		if (size == WHOLE_SIZE) size = totalSize;
		CORE_ASSERT(offset + size <= totalSize, "buffer overflow!");
		glBindBuffer(target, handle);
		auto mapped = glMapBuffer(target, GL_WRITE_ONLY);
		memcpy((char*)mapped + offset, data, size);
		glUnmapBuffer(target);
		glBindBuffer(target, 0);
	}

	// OpenGLVertexBuffer definitions
	// --------------------------------------------------

	OpenGLVertexBuffer::OpenGLVertexBuffer(void const* data, size_t size, Layout const& layout, BufferUsage usage)
		: m_layout(layout)
		, m_vertex_count((uint32_t)(size / layout.m_stride)) {
		m_size = size;
		m_handle = create_opengl_buffer(GL_ARRAY_BUFFER, data, size, usage);
	}

	OpenGLVertexBuffer::~OpenGLVertexBuffer() {
		if (m_handle == 0) return;
		glDeleteBuffers(1, &m_handle);
	}

	void OpenGLVertexBuffer::bind() const {
		glBindBuffer(GL_ARRAY_BUFFER, m_handle);
	}

	void OpenGLVertexBuffer::unbind() const {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void OpenGLVertexBuffer::write(void const* data, size_t size, size_t offset) {
		PROFILE_FUNCTION();
		write_opengl_buffer(GL_ARRAY_BUFFER, m_handle, data, size, offset, m_size);
	}

	uint32_t OpenGLVertexBuffer::bind_attributes(uint32_t start) {
		glBindBuffer(GL_ARRAY_BUFFER, m_handle);
		uint32_t location = start;
		for (auto const& element : m_layout.m_elements) {
			int32_t count = (int32_t)get_data_type_element_count(element.m_type);
			switch (element.m_type)
			{
			case DataType::Float:
			case DataType::Float2:
			case DataType::Float3:
			case DataType::Float4:
			case DataType::Bool:
				glEnableVertexAttribArray(location);
				glVertexAttribPointer(location, count, data_type_to_opengl_type(element.m_type), element.m_normalized ? GL_TRUE : GL_FALSE, (GLsizei)m_layout.m_stride, (void*)(uint64_t)element.m_offset);
				location++;
				break;
			case DataType::Int:
			case DataType::Int2:
			case DataType::Int3:
			case DataType::Int4:
				glEnableVertexAttribArray(location);
				glVertexAttribIPointer(location, count, data_type_to_opengl_type(element.m_type), (GLsizei)m_layout.m_stride, (void*)(uint64_t)element.m_offset);
				location++;
				break;
			case DataType::Mat3:
			case DataType::Mat4:
				for (int32_t i = 0; i < count; i++) {
					glEnableVertexAttribArray(location);
					glVertexAttribPointer(location, count, data_type_to_opengl_type(element.m_type), element.m_normalized ? GL_TRUE : GL_FALSE, (GLsizei)m_layout.m_stride, (void*)(element.m_offset + sizeof(float) * count * i));
					location++;
				}
				break;
			}
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		return location;
	}

	uint32_t OpenGLVertexBuffer::bind_instance_attributes(uint32_t start, uint32_t divisor) {
		glBindBuffer(GL_ARRAY_BUFFER, m_handle);
		DEBUG_RUN(glCheckError());
		uint32_t location = start;
		for (auto const& element : m_layout.m_elements) {
			int32_t count = (int32_t)get_data_type_element_count(element.m_type);
			switch (element.m_type)
			{
			case DataType::Float:
			case DataType::Float2:
			case DataType::Float3:
			case DataType::Float4:
			case DataType::Bool:
				glEnableVertexAttribArray(location);
				glVertexAttribPointer(location, count, data_type_to_opengl_type(element.m_type), element.m_normalized ? GL_TRUE : GL_FALSE, (GLsizei)m_layout.m_stride, (void*)(uint64_t)element.m_offset);
				glVertexAttribDivisor(location, divisor);
				DEBUG_RUN(glCheckError());
				location++;
				break;
			case DataType::Int:
			case DataType::Int2:
			case DataType::Int3:
			case DataType::Int4:
				glEnableVertexAttribArray(location);
				glVertexAttribIPointer(location, count, data_type_to_opengl_type(element.m_type), (GLsizei)m_layout.m_stride, (void*)(uint64_t)element.m_offset);
				glVertexAttribDivisor(location, divisor);
				DEBUG_RUN(glCheckError());
				location++;
				break;
			case DataType::Mat3:
			case DataType::Mat4:
				for (int32_t i = 0; i < count; i++) {
					glEnableVertexAttribArray(location);
					glVertexAttribPointer(location, count, data_type_to_opengl_type(element.m_type), element.m_normalized ? GL_TRUE : GL_FALSE, (GLsizei)m_layout.m_stride, (void*)(element.m_offset + sizeof(float) * count * i));
					glVertexAttribDivisor(location, divisor);
					DEBUG_RUN(glCheckError());
					location++;
				}
				break;
			}
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		return location;
	}

	// OpenGLIndexBuffer definitions
	// --------------------------------------------------

	OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t const* data, size_t size, BufferUsage usage)
		: m_index_count((uint32_t)(size / sizeof(uint32_t))) {
		m_size = size;
		m_handle = create_opengl_buffer(GL_ELEMENT_ARRAY_BUFFER, data, size, usage);
	}

	OpenGLIndexBuffer::~OpenGLIndexBuffer() {
		if (m_handle == 0) return;
		glDeleteBuffers(1, &m_handle);
	}

	void OpenGLIndexBuffer::bind() const {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_handle);
	}

	void OpenGLIndexBuffer::unbind() const {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void OpenGLIndexBuffer::write(uint32_t const* data, size_t size, size_t offset) {
		PROFILE_FUNCTION();
		write_opengl_buffer(GL_ELEMENT_ARRAY_BUFFER, m_handle, data, size, offset, m_size);
	}

	// OpenGLUniformBuffer definitions
	// --------------------------------------------------

	OpenGLUniformBuffer::OpenGLUniformBuffer(void const* data, size_t size, BufferUsage usage) {
		m_size = size;
		m_handle = create_opengl_buffer(GL_UNIFORM_BUFFER, data, size, usage);
	}

	OpenGLUniformBuffer::~OpenGLUniformBuffer() {
		if (m_handle == 0) return;
		glDeleteBuffers(1, &m_handle);
	}

	void OpenGLUniformBuffer::bind(uint32_t binding) const {
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_handle);
	}

	void OpenGLUniformBuffer::unbind(uint32_t binding) const {
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, 0);
	}

	//void OpenGLUniformBuffer::bind() const {
	//	glBindBuffer(GL_UNIFORM_BUFFER, m_handle);
	//}

	//void OpenGLUniformBuffer::unbind() const {
	//	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	//}

	void OpenGLUniformBuffer::write(void const* data, size_t size, size_t offset) {
		PROFILE_FUNCTION();
		write_opengl_buffer(GL_UNIFORM_BUFFER, m_handle, data, size, offset, m_size);
	}

}
