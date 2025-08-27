#include "pch.h"
#include "io/importer/obj_importer.h"

#include "tinyobjloader/tiny_obj_loader.h"

namespace z1::io {

	bool ObjImporter::can_import(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> exts = { ".obj" };
		return std::find(exts.begin(), exts.end(), ext) != exts.end();
	}

	static std::shared_ptr<StaticMesh::Storage> import_obj_as_mesh_storage(Filepath const& path) {
		tinyobj::ObjReaderConfig readerConfig;
		readerConfig.mtl_search_path = path.parent_path().string() + "/"; // Path to .mtl file, relative to .obj file

		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(path.string(), readerConfig)) {
			if (!reader.Error().empty()) {
				CORE_ERROR("failed to load static mesh: {0}", path);
				CORE_ERROR("TinyObjReader: {0}", reader.Error());
			}
			return nullptr;
		}

		if (!reader.Warning().empty()) {
			CORE_WARN("TinyObjReader: {0}", reader.Warning());
		}

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		auto& materials = reader.GetMaterials();

		auto mesh_storage = std::make_shared<StaticMesh::Storage>();
		mesh_storage->bound_min = glm::vec3{ FLT_MAX };
		mesh_storage->bound_max = glm::vec3{ FLT_MIN };

		uint32_t prim_index_start = 0;
		uint32_t base_index_offset = 0;

		// loop over shapes
		for (size_t s = 0; s < shapes.size(); ++s) {
			// loop over faces(polygon)

			StaticMesh::Primitive::Storage prim_storage{};
			prim_storage.bound_min = glm::vec3{ FLT_MAX };
			prim_storage.bound_max = glm::vec3{ FLT_MIN };
			prim_storage.index_start = prim_index_start;
			prim_storage.index_count = 0;
			prim_storage.vertex_count = 0;

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

					mesh_storage->vertices.push_back(vdata);

					prim_storage.vertex_count += 1;
					prim_storage.bound_min = glm::min(prim_storage.bound_min, vdata.position);
					prim_storage.bound_max = glm::max(prim_storage.bound_max, vdata.position);
				}

				for (uint32_t v = 2; v < fv; ++v) {
					// triangulate the face
					mesh_storage->indices.push_back(base_index_offset + 0);
					mesh_storage->indices.push_back(base_index_offset + v - 1);
					mesh_storage->indices.push_back(base_index_offset + v);
					prim_storage.index_count += 3;
				}

				base_index_offset += static_cast<uint32_t>(fv);
				index_offset += fv;

				// per-face material
				// shapes[s].mesh.material_ids[f];
			}

			prim_index_start += prim_storage.index_count;
			if (prim_storage.vertex_count) {
				// update mesh bounds
				mesh_storage->bound_min = glm::min(mesh_storage->bound_min, prim_storage.bound_min);
				mesh_storage->bound_max = glm::max(mesh_storage->bound_max, prim_storage.bound_max);
				mesh_storage->primitives.push_back(prim_storage);
			}
		}

		if (mesh_storage->vertices.empty() || mesh_storage->primitives.empty()) {
			CORE_ERROR("failed to load static mesh: {0} - no vertices or primitives found", path);
			return nullptr;
		}

		return mesh_storage;
	}

	ImportResult ObjImporter::import(ObjImporterSettings const& settings) {
		ImportResult ret{};
		if (!can_import(settings.file)) {
			CORE_WARN("{0} is not a common obj file", settings.file.generic_string());
			return ret;
		}

		auto const& root = FileSystem::s_content_root;

		Filepath import_file = root / settings.path;
		import_file += ".bin";
		Filepath import_meta = import_file;
		import_meta += ".meta.yaml";

		AssetMetaData meta{};
		meta.guid = Guid::generate();
		meta.type = "static mesh";
		meta.path = settings.path;

		if (!g_runtime_context.m_asset_manager->register_asset(meta, root)) {
			return ret;
		}

		auto mesh_storage = import_obj_as_mesh_storage(settings.file);

		if (mesh_storage && io::save_static_mesh_storage(import_file, mesh_storage)) {
			meta.save(import_meta);
			ret.assets.push_back(meta);
			ret.files.push_back(import_file);
			ret.success = true;
		}

		return ret;
	}

}
