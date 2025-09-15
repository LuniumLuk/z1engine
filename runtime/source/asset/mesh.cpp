#include "pch.h"
#include "asset/mesh.h"

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
		Primitive prim{ type, VertexArray::create({ vertex_buffer }), prim_min, prim_max };
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

	std::shared_ptr<StaticMesh> StaticMesh::create(Filepath const& path, std::shared_ptr<Storage> const& storage) {
		BinaryFile bf{};

		auto vdata_size = storage->vertices.size() * sizeof(StaticMesh::VertexData);
		auto idata_size = storage->indices.size() * sizeof(uint32_t);

		bf.reserve(vdata_size + idata_size);
		bf.set_data(storage->vertices.data(), vdata_size, 0);
		bf.set_data(storage->indices.data(), idata_size, vdata_size);

		YAML::Emitter yaml;
		yaml << YAML::BeginMap;
		yaml << YAML::Key << "guid" << YAML::Value << storage->guid.value;
		yaml << YAML::Key << "bound_min" << YAML::Value << storage->bound_min;
		yaml << YAML::Key << "bound_max" << YAML::Value << storage->bound_max;
		yaml << YAML::Key << "vertex_count" << YAML::Value << storage->vertices.size();
		yaml << YAML::Key << "index_count" << YAML::Value << storage->indices.size();
		yaml << YAML::Key << "primitives" << YAML::Value << YAML::BeginSeq;

		for (auto const& prim : storage->primitives) {
			yaml << YAML::BeginMap;
			yaml << YAML::Key << "index_start" << YAML::Value << prim.index_start;
			yaml << YAML::Key << "index_count" << YAML::Value << prim.index_count;
			yaml << YAML::Key << "vertex_count" << YAML::Value << prim.vertex_count;
			yaml << YAML::Key << "bound_min" << YAML::Value << prim.bound_min;
			yaml << YAML::Key << "bound_max" << YAML::Value << prim.bound_max;
			yaml << YAML::Key << "guid" << YAML::Value << prim.material.value;
			yaml << YAML::Key << "has_indices" << YAML::Value << prim.has_indices;
			yaml << YAML::Key << "has_normal" << YAML::Value << prim.has_normal;
			yaml << YAML::Key << "has_tangent" << YAML::Value << prim.has_tangent;
			yaml << YAML::EndMap;
		}

		yaml << YAML::EndMap;

		auto material = std::make_shared<StaticMesh>(storage);
		material->m_meta.guid = Guid::generate();
		material->m_meta.type = "static mesh";
		material->m_meta.path = path;
		material->m_meta.root = "content";
		auto const& root = FileSystem::s_content_root;
		if (!g_runtime_context.m_asset_manager->register_asset(material->m_meta, root)) {
			return nullptr;
		}
		material->save();

		bf.set_yaml(yaml.c_str());
		if (!bf.save(path)) {
			CORE_ERROR("failed to save static mesh storage: {0}", path.generic_string());
			return nullptr;
		}

		return material;
	}

	std::shared_ptr<StaticMesh> StaticMesh::load(Guid const& guid) {

	}

	void StaticMesh::save() const {
		auto const& root = FileSystem::s_content_root;
		Filepath file = root / m_meta.path;
		file += ".yaml";

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "meta" << YAML::Value << m_meta;
		out << YAML::Key << "pipeline" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "depth_test" << YAML::Value << m_pipeline_desc.depth_test;
		out << YAML::Key << "blend" << YAML::Value << m_pipeline_desc.blend;
		out << YAML::Key << "src_blend_factor" << YAML::Value << static_cast<int>(m_pipeline_desc.src_blend_factor);
		out << YAML::Key << "dst_blend_factor" << YAML::Value << static_cast<int>(m_pipeline_desc.dst_blend_factor);
		out << YAML::Key << "cull_mode" << YAML::Value << static_cast<int>(m_pipeline_desc.cull_mode);
		out << YAML::Key << "shader" << YAML::Value << m_pipeline_desc.shader->m_guid.value;
		out << YAML::EndMap;

		save_yaml(file, out);
	}

}
