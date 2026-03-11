#pragma once

#include "asset/asset.h"
#include "asset/material.h"
#include "render/vertex_array.h"
#include "render/buffer.h"
#include "glm/glm.hpp"

namespace z1 {

	using MaterialFlagsFilter = std::function<bool(uint32_t)>;

	struct API StaticMesh : Asset<StaticMesh> {
		using IndexType = uint32_t;
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
				Guid material;
				bool has_indices;
				bool has_normal;
				bool has_tangent;
			};

			PrimitiveType m_primitive_type;
			std::shared_ptr<VertexArray> m_vertex_array;
			glm::vec3 m_bound_min;
			glm::vec3 m_bound_max;
			Guid m_material;

			Primitive(PrimitiveType type, std::shared_ptr<VertexArray> const& vertex_array, glm::vec3 const& bound_min, glm::vec3 const& bound_max, Guid material)
				: m_primitive_type(type), m_vertex_array(vertex_array), m_bound_min(bound_min), m_bound_max(bound_max), m_material(material) {}

			Primitive(PrimitiveType type, std::shared_ptr<VertexArray> const& vertex_array)
				: m_primitive_type(type), m_vertex_array(vertex_array), m_bound_min(0.0f), m_bound_max(0.0f), m_material() {}

			size_t get_triangle_count() const;

			bool is_bounding_box_valid() const { return m_bound_min != m_bound_max; }
		};

		struct Storage {
			std::vector<VertexData> vertices;
			std::vector<uint32_t> indices;
			std::vector<Primitive::Storage> primitives;
			glm::vec3 bound_min;
			glm::vec3 bound_max;

			AssetMeta import(Filepath const& path) const;
		};

		static std::shared_ptr<StaticMesh> load(Guid const& guid);

		StaticMesh(std::shared_ptr<Storage> const& storage);
		StaticMesh(std::vector<Primitive> const& primitives);
		StaticMesh(std::vector<Primitive> const& primitives, glm::vec3 const& bound_min, glm::vec3 const& bound_max);
		StaticMesh(std::vector<VertexData> const& vertices, PrimitiveType type);
		StaticMesh(std::vector<VertexData> const& vertices, std::vector<uint32_t> const& indices, PrimitiveType type);

		void draw() const;
		void draw_instanced(
			uint32_t num,
			std::shared_ptr<VertexBuffer> const& instance_buffer,
			uint32_t start,
			uint32_t divisor) const;

		void draw(
			PerFrameConst const& per_frame,
			std::shared_ptr<MaterialInstance> const& default_material = nullptr) const;

		void draw(
			PerFrameConst const& per_frame,
			std::shared_ptr<MaterialInstance> const& default_material,
			MaterialFlagsFilter filter = nullptr) const;

		void draw_primitive(
			size_t index,
			PerFrameConst const& per_frame,
			std::shared_ptr<MaterialInstance> const& default_material = nullptr,
			MaterialFlagsFilter filter = nullptr) const;

		std::vector<Primitive> m_primitives;

		// axis-aligned bounding box for the whole mesh
		glm::vec3 m_bound_min;
		glm::vec3 m_bound_max;
	};

	struct API SkeletalMesh : Asset<SkeletalMesh> {
		using IndexType = uint32_t;
		struct VertexData {
			glm::vec3 position{ 0.0f };
			glm::vec3 normal{ 0.0f, 0.0f, 1.0f };
			glm::vec2 texcoord0{ 0.0f };
			glm::vec2 texcoord1{ 0.0f };
			glm::vec4 tangent{ 1.0f, 0.0f, 0.0f, 0.0f };
			glm::vec4 color{ 1.0f };
			glm::vec4 joint{ 0.0f };
			glm::vec4 weight{ 0.0f };
			VertexData() = default;
			VertexData(
				glm::vec3 const& pos,
				glm::vec3 const& norm,
				glm::vec2 const& tex0,
				glm::vec2 const& tex1,
				glm::vec4 const& tan,
				glm::vec4 const& col,
				glm::vec4 const& joi,
				glm::vec4 const& wei)
				: position(pos)
				, normal(norm)
				, texcoord0(tex0)
				, texcoord1(tex1)
				, tangent(tan)
				, color(col)
				, joint(joi)
				, weight(wei) {
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
				Guid material;
				bool has_indices;
				bool has_normal;
				bool has_tangent;
			};
			PrimitiveType m_primitive_type;
			std::shared_ptr<VertexArray> m_vertex_array;
			glm::vec3 m_bound_min;
			glm::vec3 m_bound_max;
			Guid m_material;

			Primitive(PrimitiveType type, std::shared_ptr<VertexArray> const& vertex_array, glm::vec3 const& bound_min, glm::vec3 const& bound_max, Guid material)
				: m_primitive_type(type), m_vertex_array(vertex_array), m_bound_min(bound_min), m_bound_max(bound_max), m_material(material) {}

			size_t get_triangle_count() const;
			bool is_bounding_box_valid() const { return m_bound_min != m_bound_max; }
		};

		struct Storage {
			std::vector<VertexData> vertices;
			std::vector<uint32_t> indices;
			std::vector<Primitive::Storage> primitives;
			glm::vec3 bound_min;
			glm::vec3 bound_max;

			AssetMeta import(Filepath const& path) const;
		};

		static std::shared_ptr<SkeletalMesh> load(Guid const& guid);

		SkeletalMesh(std::shared_ptr<Storage> const& storage);

		void draw(
			PerFrameConst const& per_frame,
			std::shared_ptr<MaterialInstance> const& default_material = nullptr,
			std::shared_ptr<UniformBuffer> const& bones = nullptr) const;

		void draw(
			PerFrameConst const& per_frame,
			std::shared_ptr<MaterialInstance> const& default_material,
			std::shared_ptr<UniformBuffer> const& bones,
			MaterialFlagsFilter filter = nullptr) const;

		void draw_primitive(
			size_t index,
			PerFrameConst const& per_frame,
			std::shared_ptr<MaterialInstance> const& default_material = nullptr,
			std::shared_ptr<UniformBuffer> const& bones = nullptr,
			MaterialFlagsFilter filter = nullptr) const;

		void draw() const;

		std::vector<Primitive> m_primitives;
		glm::vec3 m_bound_min;
		glm::vec3 m_bound_max;
	};

}
