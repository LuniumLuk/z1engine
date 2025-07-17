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

	StaticMesh::StaticMesh(std::vector<Primitive> const& primitives, glm::vec3 const& bound_min, glm::vec3 const& bound_max)
		: m_primitives(primitives), m_bound_min(bound_min), m_bound_max(bound_max) {}

	StaticMesh::StaticMesh(std::vector<Primitive> const& primitives)
		: m_primitives(primitives) {
		m_bound_min = glm::vec3(std::numeric_limits<float>::max());
		m_bound_max = glm::vec3(std::numeric_limits<float>::min());
		for (auto const& prim : m_primitives) {
			if (prim.is_bounding_box_valid()) {
				m_bound_min = glm::min(m_bound_min, prim.m_bound_min);
				m_bound_max = glm::max(m_bound_max, prim.m_bound_max);
			}
		}
	}

	StaticMesh::StaticMesh(std::vector<VertexData> const& vertices, PrimitiveType type) {
		auto vertex_buffer = VertexBuffer::create((void*)vertices.data(), vertices.size() * sizeof(VertexData),
			{
				{DataType::Float3},
				{DataType::Float3},
				{DataType::Float2},
				{DataType::Float4},
			}, BufferUsage::Static);

		glm::vec3 prim_min(0.0f), prim_max(0.0f);
		if (!vertices.empty()) {
			prim_min = prim_max = vertices[0].position;
			for (auto const& v : vertices) {
				prim_min = glm::min(prim_min, v.position);
				prim_max = glm::max(prim_max, v.position);
			}
		}
		Primitive prim{ type, VertexArray::create({ vertex_buffer }), prim_min, prim_max };
		m_primitives.push_back(prim);
		m_bound_min = prim_min;
		m_bound_max = prim_max;
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

		glm::vec3 prim_min(0.0f), prim_max(0.0f);
		if (!vertices.empty()) {
			prim_min = prim_max = vertices[0].position;
			for (auto const& v : vertices) {
				prim_min = glm::min(prim_min, v.position);
				prim_max = glm::max(prim_max, v.position);
			}
		}
		Primitive prim{ type,  VertexArray::create({ vertex_buffer }, index_buffer), prim_min, prim_max };
		m_primitives.push_back(prim);
		m_bound_min = prim_min;
		m_bound_max = prim_max;
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
