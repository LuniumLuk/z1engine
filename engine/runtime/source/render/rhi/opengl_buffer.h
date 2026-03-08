#pragma once

#include "render/buffer.h"

namespace z1 {

	struct OpenGLVertexBuffer : VertexBuffer {
		OpenGLVertexBuffer(void const* data, size_t size, Layout const& layout, BufferUsage usage);
		~OpenGLVertexBuffer() override;

		void bind() const override;
		void unbind() const override;
		void write(void const* data, size_t size, size_t offset) override;

		void* get_native_handle() const override { return (void*)(uint64_t)m_handle; }
		uint32_t get_vertex_count() const { return m_vertex_count; }
		uint32_t bind_attributes(uint32_t start = 0);
		uint32_t bind_instance_attributes(uint32_t start, uint32_t divisor);

	private:
		uint32_t m_vertex_count;
		uint32_t m_handle = 0;
		Layout m_layout;
	};

	struct OpenGLIndexBuffer : IndexBuffer {
		OpenGLIndexBuffer(uint32_t const* data, size_t size, BufferUsage usage);
		~OpenGLIndexBuffer() override;

		void bind() const override;
		void unbind() const override;
		void write(uint32_t const* data, size_t size, size_t offset) override;

		void* get_native_handle() const override { return (void*)(uint64_t)m_handle; }
		uint32_t get_index_count() const { return m_index_count; }

	private:
		uint32_t m_index_count;
		uint32_t m_handle = 0;
	};

	struct OpenGLUniformBuffer : UniformBuffer {
		OpenGLUniformBuffer(void const* data, size_t size, BufferUsage usage);
		~OpenGLUniformBuffer() override;

		void bind(uint32_t binding) const override;
		void unbind(uint32_t binding) const override;

		void write(void const* data, size_t size, size_t offset) override;

		void* get_native_handle() const override { return (void*)(uint64_t)m_handle; }

	private:
		uint32_t m_handle = 0;
	};

}
