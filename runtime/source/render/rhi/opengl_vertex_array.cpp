#include "pch.h"
#include "render/rhi/opengl_vertex_array.h"
#include "render/rhi/opengl_buffer.h"
#include "glad/glad.h"

namespace z1 {

	// helper functions
	// --------------------------------------------------

	static GLenum primitive_type_to_opengl_type(PrimitiveType type) {
		switch (type) {
		case PrimitiveType::Points: return GL_POINTS;
		case PrimitiveType::Lines: return GL_LINES;
		case PrimitiveType::LineStrip: return GL_LINE_STRIP;
		case PrimitiveType::LineStripAdjacency: return GL_LINE_STRIP_ADJACENCY;
		case PrimitiveType::LinesAdjacency: return GL_LINES_ADJACENCY;
		case PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
		case PrimitiveType::TriangleFan: return GL_TRIANGLE_FAN;
		case PrimitiveType::Triangles: return GL_TRIANGLES;
		case PrimitiveType::TriangleStripAdjacency: return GL_TRIANGLE_STRIP_ADJACENCY;
		case PrimitiveType::TrianglesAdjacency: return GL_TRIANGLES_ADJACENCY;
		case PrimitiveType::Patches: return GL_PATCHES;
		}
		CORE_ASSERT(false, "Unknown primitive type!");
		return 0;
	}

	// OpenGLVertexArray definitions
	// --------------------------------------------------

	OpenGLVertexArray::OpenGLVertexArray(std::vector<std::shared_ptr<VertexBuffer>> const& vertex_buffers, std::shared_ptr<IndexBuffer> const& index_buffer)
		: m_vertex_buffers(vertex_buffers)
		, m_index_buffer(index_buffer) {
		glGenVertexArrays(1, &m_handle);
		CORE_ASSERT(m_handle, "failed to create opengl vertexArray!");
		glBindVertexArray(m_handle);

		m_element_count = std::numeric_limits<uint32_t>::max();

		uint32_t start = 0;
		for (auto& vertex_buffer : m_vertex_buffers) {
			auto vb = dynamic_cast<OpenGLVertexBuffer*>(vertex_buffer.get());
			vb->bind();
			start = vb->bind_attributes(start);
			m_element_count = std::min(m_element_count, vb->get_vertex_count());
		}

		if (m_index_buffer) {
			auto ib = dynamic_cast<OpenGLIndexBuffer*>(m_index_buffer.get());
			m_has_indices = true;
			ib->bind();
			m_element_count = ib->get_index_count();
		}

		glBindVertexArray(0);
	}

	OpenGLVertexArray::~OpenGLVertexArray() {
		if (m_handle == 0) return;
		glDeleteVertexArrays(1, &m_handle);
	}

	void OpenGLVertexArray::bind() const {
		glBindVertexArray(m_handle);
	}

	void OpenGLVertexArray::unbind() const {
		glBindVertexArray(0);
	}

	void OpenGLVertexArray::draw(PrimitiveType type, uint32_t num) {
		PROFILE_FUNCTION();
		if (m_has_indices) {
			glDrawElements(primitive_type_to_opengl_type(type), num == NUM_MAX ? m_element_count : num, GL_UNSIGNED_INT, 0);
		}
		else {
			glDrawArrays(primitive_type_to_opengl_type(type), 0, num == NUM_MAX ? m_element_count : num);
		}
	}

	void OpenGLVertexArray::draw_instanced(PrimitiveType type, uint32_t num, std::shared_ptr<VertexBuffer> const& instance_buffer, uint32_t start, uint32_t divisor) {
		PROFILE_FUNCTION();
		if (instance_buffer) {
			auto ib = dynamic_cast<OpenGLVertexBuffer*>(instance_buffer.get());
			ib->bind_instance_attributes(start, divisor);
		}

		if (m_has_indices) {
			glDrawElementsInstanced(primitive_type_to_opengl_type(type), (uint32_t)m_element_count, GL_UNSIGNED_INT, 0, num);
		}
		else {
			glDrawArraysInstanced(primitive_type_to_opengl_type(type), 0, (uint32_t)m_element_count, num);
		}
	}

}