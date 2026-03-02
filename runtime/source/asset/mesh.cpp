#include "pch.h"
#include "asset/mesh.h"
#include "asset/asset_manager.h"
#include "asset/binary_file.h"

namespace z1 {

	VertexBuffer::Layout const StaticMesh::VertexData::s_layout = {
			{DataType::Float3},
			{DataType::Float3},
			{DataType::Float2},
			{DataType::Float2},
			{DataType::Float4},
			{DataType::Float4},
	};

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

	StaticMesh::StaticMesh(std::shared_ptr<Storage> const& storage)
		: m_bound_min(storage->bound_min)
		, m_bound_max(storage->bound_max) {
		auto vertex_buffer = VertexBuffer::create(
			storage->vertices.data(),
			storage->vertices.size() * sizeof(StaticMesh::VertexData),
			StaticMesh::VertexData::s_layout, BufferUsage::Static);
		for (auto const& prim_storage : storage->primitives) {
			auto& indices = prim_storage.index_start;
			auto index_buffer = IndexBuffer::create(
				&storage->indices[prim_storage.index_start],
				prim_storage.index_count * sizeof(uint32_t), BufferUsage::Static);
			StaticMesh::Primitive prim{
				PrimitiveType::Triangles,
				VertexArray::create({ vertex_buffer }, index_buffer),
				prim_storage.bound_min,
				prim_storage.bound_max,
				prim_storage.material,
			};
			m_primitives.push_back(prim);
		}
	}

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

	StaticMesh::StaticMesh(std::vector<Primitive> const& primitives, glm::vec3 const& bound_min, glm::vec3 const& bound_max)
		: m_primitives(primitives), m_bound_min(bound_min), m_bound_max(bound_max) {}

	StaticMesh::StaticMesh(std::vector<VertexData> const& vertices, PrimitiveType type) {
		auto vertex_buffer = VertexBuffer::create((void*)vertices.data(), vertices.size() * sizeof(VertexData),
			StaticMesh::VertexData::s_layout, BufferUsage::Static);

		glm::vec3 prim_min(0.0f), prim_max(0.0f);
		if (!vertices.empty()) {
			prim_min = prim_max = vertices[0].position;
			for (auto const& v : vertices) {
				prim_min = glm::min(prim_min, v.position);
				prim_max = glm::max(prim_max, v.position);
			}
		}
		Primitive prim{ type, VertexArray::create({ vertex_buffer }), prim_min, prim_max, {}, };
		m_primitives.push_back(prim);
		m_bound_min = prim_min;
		m_bound_max = prim_max;
	}

	StaticMesh::StaticMesh(std::vector<VertexData> const& vertices, std::vector<uint32_t> const& indices, PrimitiveType type) {
		auto vertex_buffer = VertexBuffer::create((void*)vertices.data(), vertices.size() * sizeof(VertexData),
			StaticMesh::VertexData::s_layout, BufferUsage::Static);
		auto index_buffer = IndexBuffer::create(indices.data(), indices.size() * sizeof(int), BufferUsage::Static);

		glm::vec3 prim_min(0.0f), prim_max(0.0f);
		if (!vertices.empty()) {
			prim_min = prim_max = vertices[0].position;
			for (auto const& v : vertices) {
				prim_min = glm::min(prim_min, v.position);
				prim_max = glm::max(prim_max, v.position);
			}
		}
		Primitive prim{ type,  VertexArray::create({ vertex_buffer }, index_buffer), prim_min, prim_max, {}, };
		m_primitives.push_back(prim);
		m_bound_min = prim_min;
		m_bound_max = prim_max;
	}

	void StaticMesh::draw(
		PerFrameConst const& per_frame,
		std::shared_ptr<MaterialInstance> const& default_material) const {
		for (auto const& prim : m_primitives) {
			std::shared_ptr<MaterialInstance> mi = nullptr;
			if (prim.m_material.is_valid()) {
				mi = g_runtime_context.m_asset_manager->get<MaterialInstance>(prim.m_material);
			}
			else if (default_material) {
				mi = default_material;
			}

			if (mi)
				mi->bind(per_frame);

				// If renderer produced a shadow map, bind it to the material shader as u_shadow_map
				auto shadow_img = (g_runtime_context.m_renderer_forward ? g_runtime_context.m_renderer_forward->get_shadow_image() : nullptr);
				if (shadow_img && mi && mi->m_material && mi->m_material->m_pipeline && mi->m_material->m_pipeline->m_shader) {
					shadow_img->bind(mi->m_material->m_pipeline->m_shader, "u_shadow_map");
				}
			prim.m_vertex_array->bind();
			prim.m_vertex_array->draw(prim.m_primitive_type);
			prim.m_vertex_array->unbind();

			if (shadow_img && mi && mi->m_material && mi->m_material->m_pipeline && mi->m_material->m_pipeline->m_shader) {
				shadow_img->unbind();
			}

			if (mi)
				mi->unbind();
		}
	}

	void StaticMesh::draw() const {
		for (auto const& prim : m_primitives) {
			prim.m_vertex_array->bind();
			prim.m_vertex_array->draw(prim.m_primitive_type);
			prim.m_vertex_array->unbind();
		}
	}

	void StaticMesh::draw_instanced(
		uint32_t num,
		std::shared_ptr<VertexBuffer> const& instance_buffer,
		uint32_t start,
		uint32_t divisor) const {
		for (auto const& prim : m_primitives) {
			prim.m_vertex_array->bind();
			prim.m_vertex_array->draw_instanced(prim.m_primitive_type, num, instance_buffer, start, divisor);
			prim.m_vertex_array->unbind();
		}
	}

	AssetMeta StaticMesh::Storage::import(Filepath const& path) const {
		BinaryFile bf{};

		auto vdata_size = vertices.size() * sizeof(StaticMesh::VertexData);
		auto idata_size = indices.size() * sizeof(uint32_t);

		bf.reserve(vdata_size + idata_size);
		bf.set_data(vertices.data(), vdata_size, 0);
		bf.set_data(indices.data(), idata_size, vdata_size);

		YAML::Emitter yaml;
		yaml << YAML::BeginMap;
		yaml << YAML::Key << "bound_min" << YAML::Value << bound_min;
		yaml << YAML::Key << "bound_max" << YAML::Value << bound_max;
		yaml << YAML::Key << "vertex_count" << YAML::Value << vertices.size();
		yaml << YAML::Key << "index_count" << YAML::Value << indices.size();
		yaml << YAML::Key << "primitives" << YAML::Value << YAML::BeginSeq;

		for (auto const& prim : primitives) {
			yaml << YAML::BeginMap;
			yaml << YAML::Key << "index_start" << YAML::Value << prim.index_start;
			yaml << YAML::Key << "index_count" << YAML::Value << prim.index_count;
			yaml << YAML::Key << "vertex_count" << YAML::Value << prim.vertex_count;
			yaml << YAML::Key << "bound_min" << YAML::Value << prim.bound_min;
			yaml << YAML::Key << "bound_max" << YAML::Value << prim.bound_max;
			yaml << YAML::Key << "material" << YAML::Value << prim.material;
			yaml << YAML::Key << "has_indices" << YAML::Value << prim.has_indices;
			yaml << YAML::Key << "has_normal" << YAML::Value << prim.has_normal;
			yaml << YAML::Key << "has_tangent" << YAML::Value << prim.has_tangent;
			yaml << YAML::EndMap;
		}

		yaml << YAML::EndSeq;

		AssetMeta meta{};
		meta.guid = Guid::generate();
		meta.type = "static mesh";
		meta.path = path;

		yaml << YAML::Key << "meta" << YAML::Value << meta;
		yaml << YAML::EndMap;

		auto const& root = FileSystem::s_content_root;
		Filepath file = root / meta.path;

		if (!g_runtime_context.m_asset_manager->register_asset(meta, root)) {
			return {};
		}

		bf.set_yaml(yaml.c_str());
		if (!bf.save(file.concat(".bin"))) {
			CORE_ERROR("failed to save static mesh storage: {0}", path.generic_string());
			return {};
		}

		save_yaml(file.replace_extension(".yaml"), yaml);

		return meta;
	}

	std::shared_ptr<StaticMesh> StaticMesh::load(Guid const& guid) {
		PROFILE_FUNCTION();
		auto file = g_runtime_context.m_asset_manager->get_file_from_guid(guid);

		BinaryFile bf{};

		if (!bf.load(file.concat(".bin"))) {
			CORE_ERROR("failed to load static mesh storage: {0}", file.generic_string());
			return nullptr;
		}

		YAML::Node node = YAML::Load(bf.get_yaml());
		auto storage = std::make_shared<StaticMesh::Storage>();

		storage->bound_min = node["bound_min"].as<glm::vec3>();
		storage->bound_max = node["bound_max"].as<glm::vec3>();
		auto vertex_count = node["vertex_count"].as<size_t>();
		auto index_count = node["index_count"].as<size_t>();

		for (auto const& prim_node : node["primitives"]) {
			StaticMesh::Primitive::Storage prim_storage{};
			prim_storage.index_start = prim_node["index_start"].as<uint32_t>();
			prim_storage.index_count = prim_node["index_count"].as<uint32_t>();
			prim_storage.vertex_count = prim_node["vertex_count"].as<uint32_t>();
			prim_storage.bound_min = prim_node["bound_min"].as<glm::vec3>();
			prim_storage.bound_max = prim_node["bound_max"].as<glm::vec3>();
			prim_storage.material = prim_node["material"].as<Guid>();
			prim_storage.has_indices = prim_node["has_indices"].as<bool>();
			prim_storage.has_normal = prim_node["has_normal"].as<bool>();
			prim_storage.has_tangent = prim_node["has_tangent"].as<bool>();
			storage->primitives.push_back(prim_storage);
		}

		storage->vertices.resize(vertex_count);
		storage->indices.resize(index_count);

		auto vdata_size = vertex_count * sizeof(StaticMesh::VertexData);
		auto idata_size = index_count * sizeof(uint32_t);

		auto vdata_slice = bf.get_data_slice(0, vdata_size);
		if (vdata_slice.size != vdata_size) {
			CORE_ERROR("corrupted static mesh storage: {0}", file.generic_string());
			return nullptr;
		}
		std::memcpy(storage->vertices.data(), vdata_slice.ptr, vdata_slice.size);

		auto idata_slice = bf.get_data_slice(vdata_size, idata_size);
		if (idata_slice.size != idata_size) {
			CORE_ERROR("corrupted static mesh storage: {0}", file.generic_string());
			return nullptr;
		}
		std::memcpy(storage->indices.data(), idata_slice.ptr, idata_slice.size);

		auto mesh = std::make_shared<StaticMesh>(storage);
		mesh->m_meta = g_runtime_context.m_asset_manager->get_meta(guid);
		return mesh;
	}

}
