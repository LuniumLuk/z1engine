#include "pch.h"
#include "io/obj_loader.h"

#include "tinyobjloader/tiny_obj_loader.h"

namespace z1::io {

	bool file_is_obj_mesh(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> exts = { ".obj" };
		return std::find(exts.begin(), exts.end(), ext) != exts.end();
	}

	std::shared_ptr<StaticMesh> load_obj_mesh(Filepath const& path) {
		tinyobj::ObjReaderConfig readerConfig;
		readerConfig.mtl_search_path = path.parent_path().string() + "/"; // Path to .mtl file, relative to .obj file

		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(path.string(), readerConfig)) {
			if (!reader.Error().empty()) {
				CORE_ERROR("failed to load static mesh: {0}", path);
				CORE_ERROR("TinyObjReader: {0}", reader.Error());
			}
			exit(1);
		}

		if (!reader.Warning().empty()) {
			CORE_WARN("TinyObjReader: {0}", reader.Warning());
		}

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		auto& materials = reader.GetMaterials();

		std::vector<StaticMesh::VertexData> vertices;
		std::vector<std::vector<uint32_t>> primitives;
		std::vector<glm::vec3> primitive_bound_mins;
		std::vector<glm::vec3> primitive_bound_maxs;
		uint32_t base_index_offset = 0;

		glm::vec3 mesh_min = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 mesh_max = glm::vec3(std::numeric_limits<float>::min());

		// loop over shapes
		for (size_t s = 0; s < shapes.size(); ++s) {
			// loop over faces(polygon)
			std::vector<uint32_t> indices;
			glm::vec3 prim_min = glm::vec3(std::numeric_limits<float>::max());
			glm::vec3 prim_max = glm::vec3(std::numeric_limits<float>::min());
			size_t index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
				size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

				// loop over vertices in the face.
				for (size_t v = 0; v < fv; ++v) {
					StaticMesh::VertexData vdata{};

					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					vdata.position.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
					vdata.position.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
					vdata.position.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

					// check if `normal_index` is zero or positive. negative = no normal data
					if (idx.normal_index >= 0) {
						vdata.normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
						vdata.normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
						vdata.normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
					}

					// check if `texcoord_index` is zero or positive. negative = no texcoord data
					if (idx.texcoord_index >= 0) {
						vdata.texcoord0.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
						vdata.texcoord0.y = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
					}

					// optional: vertex colors
					vdata.color.r = attrib.colors[3 * size_t(idx.vertex_index) + 0];
					vdata.color.g = attrib.colors[3 * size_t(idx.vertex_index) + 1];
					vdata.color.b = attrib.colors[3 * size_t(idx.vertex_index) + 2];
					vdata.color.a = 1.0f; // Default alpha value

					vertices.push_back(vdata);
					// update mesh bounds
					mesh_min = glm::min(mesh_min, vdata.position);
					mesh_max = glm::max(mesh_max, vdata.position);
					// update primitive bounds
					prim_min = glm::min(prim_min, vdata.position);
					prim_max = glm::max(prim_max, vdata.position);
				}

				for (uint32_t v = 2; v < fv; ++v) {
					// triangulate the face
					indices.push_back(base_index_offset + 0);
					indices.push_back(base_index_offset + v - 1);
					indices.push_back(base_index_offset + v);
				}

				base_index_offset += static_cast<uint32_t>(fv);
				index_offset += fv;

				// per-face material
				// shapes[s].mesh.material_ids[f];
			}

			if (!indices.empty()) {
				primitives.push_back(indices);
				primitive_bound_mins.push_back(prim_min);
				primitive_bound_maxs.push_back(prim_max);
			}
		}

		if (vertices.empty() || primitives.empty()) {
			CORE_ERROR("failed to load static mesh: {0} - no vertices or primitives found", path);
			return nullptr;
		}

		auto vertex_buffer = VertexBuffer::create(
			vertices.data(), vertices.size() * sizeof(StaticMesh::VertexData),
			StaticMesh::VertexData::s_layout, BufferUsage::Static);
		std::vector<StaticMesh::Primitive> mesh_primitives;
		for (size_t i = 0; i < primitives.size(); ++i) {
			auto& indices = primitives[i];
			auto index_buffer = IndexBuffer::create(indices.data(), indices.size() * sizeof(uint32_t), BufferUsage::Static);
			StaticMesh::Primitive prim{
				PrimitiveType::Triangles,
				VertexArray::create({ vertex_buffer }, index_buffer),
				primitive_bound_mins[i], primitive_bound_maxs[i]
			};
			mesh_primitives.push_back(prim);
		}

		return std::make_shared<StaticMesh>(mesh_primitives, mesh_min, mesh_max);
	}

}
