#pragma once

#include "core/guid.h"
#include "render/vertex_array.h"
#include "glm/glm.hpp"

namespace z1 {

	struct API SkeletalMesh {
		struct VertexData {
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec2 texcoord0;
			glm::vec2 texcoord1;
			glm::vec4 tangent;
			glm::vec4 joint;
			glm::vec4 weight;
			glm::vec4 color;
			VertexData() = default;
			VertexData(
				glm::vec3 const& pos,
				glm::vec3 const& norm,
				glm::vec2 const& tex0,
				glm::vec2 const& tex1,
				glm::vec4 const& tan,
				glm::vec4 const& joi,
				glm::vec4 const& wei,
				glm::vec4 const& col)
				: position(pos)
				, normal(norm)
				, texcoord0(tex0)
				, texcoord1(tex1)
				, tangent(tan)
				, joint(joi)
				, weight(wei)
				, color(col) {
			}
		};
	};

	struct API StaticMesh {

		struct VertexData {
			glm::vec3 position{ 0.0f };
			glm::vec3 normal{ 0.0f, 0.0f, 1.0f };
			glm::vec2 texcoord0{ 0.0f };
			glm::vec2 texcoord1{ 0.0f };
			glm::vec4 tangent{ 1.0f, 0.0f, 0.0f, 0.0f };
			glm::vec4 color{ 1.0f };
			VertexData() = default;
			VertexData(
				glm::vec3 const& pos,
				glm::vec3 const& norm,
				glm::vec2 const& tex0,
				glm::vec2 const& tex1,
				glm::vec4 const& tan,
				glm::vec4 const& col)
				: position(pos)
				, normal(norm)
				, texcoord0(tex0)
				, texcoord1(tex1)
				, tangent(tan)
				, color(col) {
			}

			static const VertexBuffer::Layout s_layout;
		};

		struct Primitive {

			struct Storage {
				uint32_t index_start;
				uint32_t index_count;
				uint32_t vertex_count;
				glm::vec3 bound_min;
				glm::vec3 bound_max;
				Guid material; // guid
				bool has_indices;
				bool has_normal;
				bool has_tangent;
			};

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

		struct Storage {
			Guid guid;
			std::vector<VertexData> vertices;
			std::vector<uint32_t> indices;
			std::vector<Primitive::Storage> primitives;
			glm::vec3 bound_min;
			glm::vec3 bound_max;
		};

		StaticMesh(std::shared_ptr<Storage> const& storage);
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
