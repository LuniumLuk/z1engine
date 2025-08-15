#pragma once

#include "core/guid.h"
#include "render/vertex_array.h"
#include "glm/glm.hpp"

namespace z1 {

	struct API StaticMesh {
		struct VertexData {
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec2 tex_coord;
			glm::vec4 color;
			VertexData() = default;
			VertexData(glm::vec3 const& pos, glm::vec3 const& norm, glm::vec2 const& tex, glm::vec4 const& col)
				: position(pos), normal(norm), tex_coord(tex), color(col) {
			}
		};

		struct Primitive {
			PrimitiveType m_primitive_type;
			std::shared_ptr<VertexArray> m_vertex_array;
			glm::vec3 m_bound_min;
			glm::vec3 m_bound_max;

			Primitive(PrimitiveType type, std::shared_ptr<VertexArray> const& vertex_array, glm::vec3 const& bound_min, glm::vec3 const& bound_max)
				: m_primitive_type(type), m_vertex_array(vertex_array), m_bound_min(bound_min), m_bound_max(bound_max) {}

			Primitive(PrimitiveType type, std::shared_ptr<VertexArray> const& vertex_array)
				: m_primitive_type(type), m_vertex_array(vertex_array), m_bound_min(0.0f), m_bound_max(0.0f) {}


			size_t get_triangle_count() const;

			bool is_bounding_box_valid() const { return m_bound_min != m_bound_max; }
		};

		StaticMesh(std::vector<Primitive> const& primitives);
		StaticMesh(std::vector<Primitive> const& primitives, glm::vec3 const& bound_min, glm::vec3 const& bound_max);
		StaticMesh(std::vector<VertexData> const& vertices, PrimitiveType type);
		StaticMesh(std::vector<VertexData> const& vertices, std::vector<uint32_t> const& indices, PrimitiveType type);

		void draw() const;
		void draw_instanced(uint32_t num, std::shared_ptr<VertexBuffer> const& instance_buffer, uint32_t start, uint32_t divisor) const;

		Guid m_guid{};
		std::vector<Primitive> m_primitives;

		// axis-aligned bounding box for the whole mesh
		glm::vec3 m_bound_min;
		glm::vec3 m_bound_max;
	};

}
