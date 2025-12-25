#pragma once

#include "core/core.h"
#include "render/resource.h"
#include "render/data_types.h"
#include <vector>

namespace z1 {

	struct Shader;

	enum struct API BufferUsage {
		Static = 0,
		Dynamic,
		Stream,
	};

	struct API VertexBuffer : RenderResource {
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

		VertexBuffer() : RenderResource(ResourceType::Buffer) {}

		virtual ~VertexBuffer() = default;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;
		virtual void write(void const* data, size_t size = WHOLE_SIZE, size_t offset = 0) = 0;

		virtual void* get_native_handle() const = 0;
		size_t get_size() const { return m_size; }

		static std::shared_ptr<VertexBuffer> create(void const* data, size_t size, Layout const& layout, BufferUsage usage = BufferUsage::Static);

	protected:
		size_t m_size = 0;
	};

	struct API IndexBuffer : RenderResource {
		IndexBuffer() : RenderResource(ResourceType::Buffer) {}

		virtual ~IndexBuffer() = default;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;
		virtual void write(uint32_t const* data, size_t size = WHOLE_SIZE, size_t offset = 0) = 0;

		virtual void* get_native_handle() const = 0;
		size_t get_size() const { return m_size; }

		static std::shared_ptr<IndexBuffer> create(uint32_t const* data, size_t size, BufferUsage usage = BufferUsage::Static);

	protected:
		size_t m_size = 0;
	};

	struct API UniformBuffer : RenderResource {
		UniformBuffer() : RenderResource(ResourceType::Buffer) {}

		virtual ~UniformBuffer() = default;

		// Automatically bind to a binding point managed globally
		void bind() const;
		// Helper function to bind to a specific shader uniform
		void bind(std::shared_ptr<Shader> const& shader, std::string const& name) const;
		// Automatically unbind from the binding point managed globally
		void unbind() const;

		bool is_bound() const { return m_binding != INVALID_BINDING; }
		// Get the current binding point
		uint32_t get_binding() const { return m_binding; }

		virtual void write(void const* data, size_t size = WHOLE_SIZE, size_t offset = 0) = 0;

		virtual void* get_native_handle() const = 0;
		size_t get_size() const { return m_size; }

		static std::shared_ptr<UniformBuffer> create(void const* data, size_t size, BufferUsage usage = BufferUsage::Static);

	protected:
		virtual void bind(uint32_t binding) const = 0;
		virtual void unbind(uint32_t binding) const = 0;

		size_t m_size = 0;
		mutable uint32_t m_binding = INVALID_BINDING;
		mutable uint32_t m_ref_count = 0;
	};

}
