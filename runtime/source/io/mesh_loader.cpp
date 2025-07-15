#include "pch.h"
#include "io/mesh_loader.h"

#include "tinyobjloader/tiny_obj_loader.h"

namespace z1::io {

	bool file_is_static_mesh(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> imageExtensions = { ".obj" };
		return std::find(imageExtensions.begin(), imageExtensions.end(), ext) != imageExtensions.end();
	}

	std::shared_ptr<StaticMesh> load_static_mesh(Filepath const& path) {
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
		uint32_t base_index_offset = 0;

		// Loop over shapes
		for (size_t s = 0; s < shapes.size(); ++s) {
			// Loop over faces(polygon)
			std::vector<uint32_t> indices;
			size_t index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
				size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; ++v) {
					StaticMesh::VertexData vdata{};

					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					vdata.position.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
					vdata.position.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
					vdata.position.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

					// Check if `normal_index` is zero or positive. negative = no normal data
					if (idx.normal_index >= 0) {
						vdata.normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
						vdata.normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
						vdata.normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
					}

					// Check if `texcoord_index` is zero or positive. negative = no texcoord data
					if (idx.texcoord_index >= 0) {
						vdata.tex_coord.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
						vdata.tex_coord.y = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
					}

					// Optional: vertex colors
					vdata.color.r = attrib.colors[3 * size_t(idx.vertex_index) + 0];
					vdata.color.g = attrib.colors[3 * size_t(idx.vertex_index) + 1];
					vdata.color.b = attrib.colors[3 * size_t(idx.vertex_index) + 2];
					vdata.color.a = 1.0f; // Default alpha value

					vertices.push_back(vdata);
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
				//shapes[s].mesh.material_ids[f];
			}

			if (!indices.empty()) {
				primitives.push_back(indices);
			}
		}

		if (vertices.empty() || primitives.empty()) {
			CORE_ERROR("failed to load static mesh: {0} - no vertices or primitives found", path);
			return nullptr;
		}

		auto vertex_buffer = VertexBuffer::create(
			vertices.data(), vertices.size() * sizeof(StaticMesh::VertexData),
			{
				{DataType::Float3},
				{DataType::Float3},
				{DataType::Float2},
				{DataType::Float4},
			}, BufferUsage::Static);
		std::vector<StaticMesh::Primitive> mesh_primitives;
		for (auto& indices : primitives) {
			auto index_buffer = IndexBuffer::create(indices.data(), indices.size() * sizeof(uint32_t), BufferUsage::Static);
			StaticMesh::Primitive prim{ PrimitiveType::Triangles, VertexArray::create({ vertex_buffer }, index_buffer) };
			mesh_primitives.push_back(prim);
		}

		return std::make_shared<StaticMesh>(mesh_primitives);
	}

}
