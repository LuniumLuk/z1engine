#include "pch.h"
#include "io/gltf_loader.h"
#include "scene/entity.h"
#include "scene/component/mesh.h"
#include "render/mesh.h"
#include "render/material.h"

#include "tinygltf/tiny_gltf.h"
#include "glm/gtc/type_ptr.hpp"

namespace z1::io {

	bool file_is_gltf(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> exts = { ".gltf", ".glb"};
		return std::find(exts.begin(), exts.end(), ext) != exts.end();
	}

	struct VertexAttribute {
		const float* data = nullptr;
		int stride = 0;
		int components = 0;
	};

	static VertexAttribute load_attribute(
		tinygltf::Model const& model,
		tinygltf::Primitive const& primitive,
		std::string const& name,
		int component_type) {
		VertexAttribute result{};
		auto it = primitive.attributes.find(name);
		if (it != primitive.attributes.end()) {
			auto const& accessor = model.accessors[it->second];
			auto const& view = model.bufferViews[accessor.bufferView];
			result.data = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
			result.components = tinygltf::GetNumComponentsInType(component_type);
			result.stride = accessor.ByteStride(view) ? (accessor.ByteStride(view) / sizeof(float)) : result.components;
		}
		return result;
	}

	struct NonFloatAttribute {
		const void* data = nullptr;
		int stride = 0;
		int components = 0;
		int component_type = 0;
	};

	static NonFloatAttribute load_non_float_attribute(
		tinygltf::Model const& model,
		tinygltf::Primitive const& primitive,
		std::string const& name,
		int component_type) {
		NonFloatAttribute result{};
		auto it = primitive.attributes.find(name);
		if (it != primitive.attributes.end()) {
			auto const& accessor = model.accessors[it->second];
			auto const& view = model.bufferViews[accessor.bufferView];
			result.data = &(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]);
			result.component_type = accessor.componentType;
			result.components = tinygltf::GetNumComponentsInType(component_type);
			int component_size = tinygltf::GetComponentSizeInBytes(result.component_type);
			result.stride = accessor.ByteStride(view) ? (accessor.ByteStride(view) / component_size) : result.components;
		}
		return result;
	}

	template<typename T>
	static void process_indices(const void* ptr, size_t count, StaticMesh::Storage& mesh_storage, uint32_t& index_pos, uint32_t vertex_start) {
		const T* buf = static_cast<const T*>(ptr);
		for (size_t i = 0; i < count; ++i) {
			mesh_storage.indices.push_back(buf[i] + vertex_start);
			++index_pos;
		}
	}

	static glm::mat4 get_transform(tinygltf::Node const& node) {
		glm::vec3 translation{ 0.0f };
		glm::quat rotation{ {0.0f, 0.0f, 0.0f} };
		glm::vec3 scale{ 1.0f };
		glm::mat4 transform{ 1.0f };
		if (node.translation.size() == 3) {
			translation = glm::make_vec3(node.translation.data());
		}
		if (node.rotation.size() == 4) {
			rotation = glm::make_quat(node.rotation.data());;
		}
		if (node.scale.size() == 3) {
			scale = glm::make_vec3(node.scale.data());
		}
		if (node.matrix.size() == 16) {
			transform = glm::make_mat4(node.matrix.data());
		};

		return glm::translate(glm::mat4{ 1.f }, translation)
			* glm::mat4(rotation)
			* glm::scale(glm::mat4{ 1.f }, scale)
			* transform;
	}

	static void load_node(
		std::shared_ptr<Scene> const& scene,
		TransformComponent* parent,
		tinygltf::Node const& node,
		tinygltf::Model const& model) {

		auto const& name = node.name;
		auto entity = scene->create_entity(name);
		auto& transform_comp = entity->get_component<TransformComponent>();
		transform_comp.m_parent = parent;
		transform_comp.set_local_transform(get_transform(node));

		if (node.children.size() > 0) {
			for (auto i : node.children) {
				load_node(scene, &transform_comp, model.nodes[i], model);
			}
		}

		StaticMesh::Storage mesh_storage{};
		mesh_storage.bound_min = glm::vec3{ FLT_MAX };
		mesh_storage.bound_max = glm::vec3{ FLT_MIN };

		uint32_t mesh_vertex_count = 0;
		uint32_t mesh_index_count = 0;
		uint32_t vertex_start = 0;
		uint32_t index_start = 0;

		if (node.mesh > -1) {
			auto const& mesh = model.meshes[node.mesh];

			// preprocess node for memory pre-allocation
			for (auto const& primitive : mesh.primitives) {
				mesh_vertex_count += static_cast<uint32_t>(model.accessors[primitive.attributes.find("POSITION")->second].count);
				if (primitive.indices > -1) {
					mesh_index_count += static_cast<uint32_t>(model.accessors[primitive.indices].count);
				}
			}

			mesh_storage.vertices.reserve(mesh_vertex_count);
			mesh_storage.indices.reserve(mesh_index_count);

			for (auto const& primitive : mesh.primitives) {

				StaticMesh::Primitive::Storage prim_storage{};

				// load vertices
				{
					if (primitive.attributes.find("POSITION") == primitive.attributes.end()) {
						CORE_WARN("skip mesh for POSITION attribute not exist");
						return;
					}

					auto const& accessor_pos = model.accessors[primitive.attributes.find("POSITION")->second];
					prim_storage.bound_min = glm::make_vec3(accessor_pos.minValues.data());
					prim_storage.bound_max = glm::make_vec3(accessor_pos.maxValues.data());
					// update mesh bound
					mesh_storage.bound_min = glm::min(mesh_storage.bound_min, prim_storage.bound_min);
					mesh_storage.bound_max = glm::max(mesh_storage.bound_max, prim_storage.bound_max);

					prim_storage.vertex_count = static_cast<uint32_t>(accessor_pos.count);

					auto pos     = load_attribute(model, primitive, "POSITION",   TINYGLTF_TYPE_VEC3);
					auto normal  = load_attribute(model, primitive, "NORMAL",     TINYGLTF_TYPE_VEC3);
					auto uv0     = load_attribute(model, primitive, "TEXCOORD_0", TINYGLTF_TYPE_VEC2);
					auto uv1     = load_attribute(model, primitive, "TEXCOORD_1", TINYGLTF_TYPE_VEC2);
					auto tangent = load_attribute(model, primitive, "TANGENT",    TINYGLTF_TYPE_VEC4);
					auto color   = load_attribute(model, primitive, "COLOR_0",    TINYGLTF_TYPE_VEC3);
					auto weight  = load_attribute(model, primitive, "WEIGHTS_0",  TINYGLTF_TYPE_VEC4);
					auto joint   = load_non_float_attribute(model, primitive, "JOINTS_0", TINYGLTF_TYPE_VEC4);

					prim_storage.has_indices = primitive.indices > -1;
					prim_storage.has_normal = (normal.data != nullptr);
					prim_storage.has_tangent = (tangent.data != nullptr);
					bool has_skin = (joint.data && weight.data);

					for (size_t v = 0; v < accessor_pos.count; ++v) {
						StaticMesh::VertexData vert{};
						vert.position = glm::vec4{ glm::make_vec3(&pos.data[v * pos.stride]), 1.f };
						if (normal.data) vert.normal = glm::normalize(glm::make_vec3(&normal.data[v * normal.stride]));
						if (uv0.data) vert.texcoord0 = glm::make_vec2(&uv0.data[v * uv0.stride]);
						if (uv1.data) vert.texcoord1 = glm::make_vec2(&uv1.data[v * uv1.stride]);
						if (tangent.data) vert.tangent = glm::make_vec4(&tangent.data[v * tangent.stride]);
						if (color.data) vert.color = glm::make_vec4(&color.data[v * color.stride]);

						//if (has_skin) {
						//	switch (joint.component_type) {
						//	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
						//		uint16_t const* buf = static_cast<uint16_t const*>(joint.data);
						//		vert.joint = glm::vec4(glm::make_vec4(&buf[v * joint.stride]));
						//		break;
						//	}
						//	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
						//		uint8_t const* buf = static_cast<uint8_t const*>(joint.data);
						//		vert.joint = glm::vec4(glm::make_vec4(&buf[v * joint.stride]));
						//		break;
						//	}
						//	default:
						//		CORE_WARN("skip mesh for unsupported joint component type");
						//		return;
						//	}
						//	vert.weight = glm::make_vec4(&weight.data[v * weight.stride]);
						//}
						//else {
						//	vert.joint = glm::vec4(0.0f);
						//	vert.weight = glm::vec4(0.0f);
						//}
						//if (glm::length(vert.weight) == 0.0f) vert.weight.x = 1.0f;

						mesh_storage.vertices.push_back(vert);
					}
				}

				// load indices
				if (prim_storage.has_indices) {
					auto const& accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
					auto const& view = model.bufferViews[accessor.bufferView];
					auto const& buffer = model.buffers[view.buffer];

					prim_storage.index_start = index_start;
					prim_storage.index_count = static_cast<uint32_t>(accessor.count);
					const void* ptr = &(buffer.data[accessor.byteOffset + view.byteOffset]);

					switch (accessor.componentType) {
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: 
						process_indices<uint32_t>(ptr, accessor.count, mesh_storage, index_start, vertex_start);
						break;
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
						process_indices<uint16_t>(ptr, accessor.count, mesh_storage, index_start, vertex_start);
						break;
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
						process_indices<uint8_t>(ptr, accessor.count, mesh_storage, index_start, vertex_start);
						break;
					default:
						CORE_WARN("skip mesh for index type not supported");
						return;
					}
				}

				mesh_storage.primitives.push_back(prim_storage);
			}

			auto vertex_buffer = VertexBuffer::create(
				mesh_storage.vertices.data(),
				mesh_storage.vertices.size() * sizeof(StaticMesh::VertexData),
				StaticMesh::VertexData::s_layout, BufferUsage::Static);
			std::vector<StaticMesh::Primitive> mesh_primitives;
			for (auto const& prim_storage : mesh_storage.primitives) {
				auto& indices = prim_storage.index_start;
				auto index_buffer = IndexBuffer::create(
					&mesh_storage.indices[prim_storage.index_start],
					prim_storage.index_count * sizeof(uint32_t), BufferUsage::Static);
				StaticMesh::Primitive prim{
					PrimitiveType::Triangles,
					VertexArray::create({ vertex_buffer }, index_buffer),
					prim_storage.bound_min,
					prim_storage.bound_max,
				};
				mesh_primitives.push_back(prim);
			}

			auto static_mesh = std::make_shared<StaticMesh>(mesh_primitives, mesh_storage.bound_min, mesh_storage.bound_max);
			entity->add_component<StaticMeshComponent>(static_mesh);
		}
	}

	void load_gltf_scene(std::shared_ptr<Scene> const& scene, Filepath const& path) {

		if (!file_is_gltf(path)) {
			CORE_WARN("{0} is not a common gltf file", path);
		}

		tinygltf::Model model{};
		tinygltf::TinyGLTF loader{};
		std::string err;
		std::string warn;

		bool result = loader.LoadBinaryFromFile(&model, &err, &warn, path.string());

		if (!err.empty()) {
			CORE_ERROR("failed to load static mesh: {0}", path);
			CORE_ERROR("TinyGLTF: {0}", err);
			return;
		}
		if (!warn.empty()) {
			CORE_WARN("TinyGLTF: {0}", warn);
		}
		if (!result) {
			CORE_ERROR("failed to load static mesh: {0}", path);
			return;
		}

		// load default scene.
		auto const& default_scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

		// nodes will be loaded as 'Entity'
		// meshes will be loaded as 'StaticMesh'
		// primitives will be loaded as 'StaticMesh::Primitive'
		for (auto i : default_scene.nodes) {
			auto const& node = model.nodes[i];
			load_node(scene, nullptr, node, model);
		}

		//loadAnimations(model);
		//loadSkins(model);

		//for (auto& node : mNodes) {
		//	if (node->skinIndex > -1) {
		//		node->skin = &mSkins[node->skinIndex];
		//	}
		//	if (node->mesh) {
		//		node->update(mTransform);
		//	}
		//}

		//mExtensions = model.extensionsUsed;
	}

}
