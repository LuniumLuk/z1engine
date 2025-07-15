#include "pch.h"
#include "render/mesh.h"

namespace z1 {

	size_t StaticMesh::Primitive::get_triangle_count() const {
		auto vcount = m_vertex_array->get_element_count();
		switch (m_primitive_type) {
		case PrimitiveType::Points: return vcount;
		case PrimitiveType::LineStrip: return vcount - 1;
		case PrimitiveType::Lines: return vcount / 2;
		case PrimitiveType::LineStripAdjacency: return (vcount - 1) / 2;
		case PrimitiveType::LinesAdjacency: return vcount / 4;
		case PrimitiveType::TriangleStrip: return vcount - 2;
		case PrimitiveType::TriangleFan: return vcount - 2;
		case PrimitiveType::Triangles: return vcount / 3;
		case PrimitiveType::TriangleStripAdjacency: return (vcount - 2) / 3;
		case PrimitiveType::TrianglesAdjacency: return vcount / 6;
		case PrimitiveType::Patches:
			// Patches are not supported in this context, return 0
			return 0;
		}

		CORE_ASSERT(false, "Unknown primitive type!");
		return 0;
	}

	StaticMesh::StaticMesh(std::vector<Primitive> const& primitives)
		: m_primitives(primitives) {
	}

	StaticMesh::StaticMesh(std::vector<VertexData> const& vertices, PrimitiveType type) {
		auto vertex_buffer = VertexBuffer::create((void*)vertices.data(), vertices.size() * sizeof(VertexData),
			{
				{DataType::Float3},
				{DataType::Float3},
				{DataType::Float2},
				{DataType::Float4},
			}, BufferUsage::Static);

		Primitive prim{ type, VertexArray::create({ vertex_buffer }) };
		m_primitives.push_back(prim);
	}

	StaticMesh::StaticMesh(std::vector<VertexData> const& vertices, std::vector<uint32_t> const& indices, PrimitiveType type) {
		auto vertex_buffer = VertexBuffer::create((void*)vertices.data(), vertices.size() * sizeof(VertexData),
			{
				{DataType::Float3},
				{DataType::Float3},
				{DataType::Float2},
				{DataType::Float4},
			}, BufferUsage::Static);
		auto index_buffer = IndexBuffer::create(indices.data(), indices.size() * sizeof(int), BufferUsage::Static);

		Primitive prim{ type,  VertexArray::create({ vertex_buffer }, index_buffer) };
		m_primitives.push_back(prim);
	}

	void StaticMesh::draw() const {
		for (auto const& prim : m_primitives) {
			prim.m_vertex_array->bind();
			prim.m_vertex_array->draw(prim.m_primitive_type);
			prim.m_vertex_array->unbind();
		}
	}

	void StaticMesh::draw_instanced(uint32_t num, std::shared_ptr<VertexBuffer> const& instance_buffer, uint32_t start, uint32_t divisor) const {
		for (auto const& prim : m_primitives) {
			prim.m_vertex_array->bind();
			prim.m_vertex_array->draw_instanced(prim.m_primitive_type, num, instance_buffer, start, divisor);
			prim.m_vertex_array->unbind();
		}
	}

}
