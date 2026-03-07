#include "pch.h"
#include "asset/importer/gltf_importer.h"
#include "scene/entity.h"
#include "scene/component/mesh.h"
#include "asset/mesh.h"
#include "asset/material.h"
#include "animation/skeleton.h"
#include "animation/animation.h"
#include "asset/binary_file.h"
#include "util/yaml.h"

#include "bakery.h"
#include "tinygltf/tiny_gltf.h"
#include "stb/stb_image.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"

namespace z1 {

	bool GltfImporter::can_import(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> exts = { ".gltf", ".glb" };
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

	template<typename MeshType, typename IndexType>
	static void process_indices(const void* ptr, size_t count, std::shared_ptr<typename MeshType::Storage> const& mesh_storage, uint32_t& index_start, uint32_t vertex_start) {
		const auto* buf = static_cast<const IndexType*>(ptr);
		for (size_t i = 0; i < count; ++i) {
			mesh_storage->indices.push_back(buf[i] + vertex_start);
			++index_start;
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
		GltfImporterSettings const& settings,
		tinygltf::Node const& node,
		tinygltf::Model const& model,
		ImportResult& ret,
		std::vector<Guid> const& loaded_materials,
		std::vector<Guid> const& loaded_skeletons,
		std::map<int, std::pair<Guid, bool>>& loaded_meshes,
		YAML::Emitter& yaml,
		int& current_id,
		int parent_id) {

		int my_id = current_id++;
		auto const& name = node.name.empty() ? ("Node_" + std::to_string(my_id)) : node.name;

		yaml << YAML::BeginMap;
		yaml << YAML::Key << "name" << YAML::Value << name;
		yaml << YAML::Key << "id" << YAML::Value << my_id;
		// Transform
		yaml << YAML::Key << "transform" << YAML::Value;
		yaml << YAML::BeginMap;

		glm::vec3 translation{ 0.0f };
		glm::quat rotation{ {0.0f, 0.0f, 0.0f} };
		glm::vec3 scale{ 1.0f };

		if (node.matrix.size() == 16) {
			glm::mat4 transform = glm::make_mat4(node.matrix.data());
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::decompose(transform, scale, rotation, translation, skew, perspective);
		}
		else {
			if (node.translation.size() == 3) translation = glm::make_vec3(node.translation.data());
			if (node.rotation.size() == 4) rotation = glm::make_quat(node.rotation.data());
			if (node.scale.size() == 3) scale = glm::make_vec3(node.scale.data());
		}

		yaml << YAML::Key << "location" << YAML::Value << translation;
		yaml << YAML::Key << "rotation" << YAML::Value << glm::degrees(glm::eulerAngles(rotation));
		yaml << YAML::Key << "scale" << YAML::Value << scale;
		if (parent_id != -1) {
			yaml << YAML::Key << "parent" << YAML::Value << parent_id;
		}
		yaml << YAML::EndMap;

		if (node.mesh > -1) {
			Guid mesh_guid;
			bool is_skeletal = false;

			if (loaded_meshes.count(node.mesh)) {
				auto pair = loaded_meshes[node.mesh];
				mesh_guid = pair.first;
				is_skeletal = pair.second;
			}
			else {
				auto const& mesh = model.meshes[node.mesh];

				for (auto const& primitive : mesh.primitives) {
					if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end() &&
						primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
						is_skeletal = true;
						break;
					}
				}

				if (is_skeletal) {
					auto mesh_storage = std::make_shared<SkeletalMesh::Storage>();
					mesh_storage->bound_min = glm::vec3{ FLT_MAX };
					mesh_storage->bound_max = glm::vec3{ FLT_MIN };

					uint32_t mesh_vertex_count = 0;
					uint32_t mesh_index_count = 0;
					uint32_t vertex_start = 0;
					uint32_t index_start = 0;

					// preprocess node for memory pre-allocation
					for (auto const& primitive : mesh.primitives) {
						mesh_vertex_count += static_cast<uint32_t>(model.accessors[primitive.attributes.find("POSITION")->second].count);
						if (primitive.indices > -1) {
							mesh_index_count += static_cast<uint32_t>(model.accessors[primitive.indices].count);
						}
					}

					mesh_storage->vertices.reserve(mesh_vertex_count);
					mesh_storage->indices.reserve(mesh_index_count);

					for (auto const& primitive : mesh.primitives) {

						uint32_t vertex_start_copy = vertex_start;
						SkeletalMesh::Primitive::Storage prim_storage{};

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
							mesh_storage->bound_min = glm::min(mesh_storage->bound_min, prim_storage.bound_min);
							mesh_storage->bound_max = glm::max(mesh_storage->bound_max, prim_storage.bound_max);

							prim_storage.vertex_count = static_cast<uint32_t>(accessor_pos.count);

							auto pos = load_attribute(model, primitive, "POSITION", TINYGLTF_TYPE_VEC3);
							auto normal = load_attribute(model, primitive, "NORMAL", TINYGLTF_TYPE_VEC3);
							auto uv0 = load_attribute(model, primitive, "TEXCOORD_0", TINYGLTF_TYPE_VEC2);
							auto uv1 = load_attribute(model, primitive, "TEXCOORD_1", TINYGLTF_TYPE_VEC2);
							auto tangent = load_attribute(model, primitive, "TANGENT", TINYGLTF_TYPE_VEC4);
							auto color = load_attribute(model, primitive, "COLOR_0", TINYGLTF_TYPE_VEC3);
							auto weight = load_attribute(model, primitive, "WEIGHTS_0", TINYGLTF_TYPE_VEC4);
							auto joint = load_non_float_attribute(model, primitive, "JOINTS_0", TINYGLTF_TYPE_VEC4);

							prim_storage.has_indices = primitive.indices > -1;
							prim_storage.has_normal = (normal.data != nullptr);
							prim_storage.has_tangent = (tangent.data != nullptr);
							bool has_skin = (joint.data && weight.data);

							for (size_t v = 0; v < accessor_pos.count; ++v) {
								SkeletalMesh::VertexData vert{};
								vert.position = glm::vec4{ glm::make_vec3(&pos.data[v * pos.stride]), 1.f };
								if (normal.data) vert.normal = glm::normalize(glm::make_vec3(&normal.data[v * normal.stride]));
								if (uv0.data) vert.texcoord0 = glm::make_vec2(&uv0.data[v * uv0.stride]);
								if (uv1.data) vert.texcoord1 = glm::make_vec2(&uv1.data[v * uv1.stride]);
								if (tangent.data) vert.tangent = glm::make_vec4(&tangent.data[v * tangent.stride]);
								if (color.data) vert.color = glm::make_vec4(&color.data[v * color.stride]);

								if (has_skin) {
									switch (joint.component_type) {
									case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
										uint16_t const* buf = static_cast<uint16_t const*>(joint.data);
										auto* ptr = &buf[v * joint.stride];
										vert.joint = glm::vec4(ptr[0], ptr[1], ptr[2], ptr[3]);
										break;
									}
									case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
										uint8_t const* buf = static_cast<uint8_t const*>(joint.data);
										auto* ptr = &buf[v * joint.stride];
										vert.joint = glm::vec4(ptr[0], ptr[1], ptr[2], ptr[3]);
										break;
									}
									default:
										CORE_WARN("skip mesh for unsupported joint component type");
										return;
									}
									vert.weight = glm::make_vec4(&weight.data[v * weight.stride]);
								}
								else {
									vert.joint = glm::vec4(0.0f);
									vert.weight = glm::vec4(0.0f);
								}
								if (glm::length(vert.weight) == 0.0f) vert.weight.x = 1.0f;

								mesh_storage->vertices.push_back(vert);
							}
							vertex_start += static_cast<uint32_t>(accessor_pos.count);
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
								process_indices<SkeletalMesh, uint32_t>(ptr, accessor.count, mesh_storage, index_start, vertex_start_copy);
								break;
							case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
								process_indices<SkeletalMesh, uint16_t>(ptr, accessor.count, mesh_storage, index_start, vertex_start_copy);
								break;
							case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
								process_indices<SkeletalMesh, uint8_t>(ptr, accessor.count, mesh_storage, index_start, vertex_start_copy);
								break;
							default:
								CORE_WARN("skip mesh for index type not supported");
								return;
							}
						}

						if (primitive.material >= 0) {
							prim_storage.material = loaded_materials[primitive.material];
						}

						mesh_storage->primitives.push_back(prim_storage);
					}

					auto const& root = FileSystem::s_content_root;

					std::string name = mesh.name.empty() ? ("SKM_" + std::to_string(node.mesh)) : mesh.name;
					auto meta = mesh_storage->import(settings.path / name);
					if (meta.guid.is_valid()) {
						ret.assets.push_back(meta);
						ret.files.push_back((root / (name + ".bin")));
						mesh_guid = meta.guid;
					}
				}
				else {
					auto mesh_storage = std::make_shared<StaticMesh::Storage>();
					mesh_storage->bound_min = glm::vec3{ FLT_MAX };
					mesh_storage->bound_max = glm::vec3{ FLT_MIN };

					uint32_t mesh_vertex_count = 0;
					uint32_t mesh_index_count = 0;
					uint32_t vertex_start = 0;
					uint32_t index_start = 0;

					// preprocess node for memory pre-allocation
					for (auto const& primitive : mesh.primitives) {
						mesh_vertex_count += static_cast<uint32_t>(model.accessors[primitive.attributes.find("POSITION")->second].count);
						if (primitive.indices > -1) {
							mesh_index_count += static_cast<uint32_t>(model.accessors[primitive.indices].count);
						}
					}

					mesh_storage->vertices.reserve(mesh_vertex_count);
					mesh_storage->indices.reserve(mesh_index_count);

					for (auto const& primitive : mesh.primitives) {

						uint32_t vertex_start_copy = vertex_start;
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
							mesh_storage->bound_min = glm::min(mesh_storage->bound_min, prim_storage.bound_min);
							mesh_storage->bound_max = glm::max(mesh_storage->bound_max, prim_storage.bound_max);

							prim_storage.vertex_count = static_cast<uint32_t>(accessor_pos.count);

							auto pos = load_attribute(model, primitive, "POSITION", TINYGLTF_TYPE_VEC3);
							auto normal = load_attribute(model, primitive, "NORMAL", TINYGLTF_TYPE_VEC3);
							auto uv0 = load_attribute(model, primitive, "TEXCOORD_0", TINYGLTF_TYPE_VEC2);
							auto uv1 = load_attribute(model, primitive, "TEXCOORD_1", TINYGLTF_TYPE_VEC2);
							auto tangent = load_attribute(model, primitive, "TANGENT", TINYGLTF_TYPE_VEC4);
							auto color = load_attribute(model, primitive, "COLOR_0", TINYGLTF_TYPE_VEC3);

							prim_storage.has_indices = primitive.indices > -1;
							prim_storage.has_normal = (normal.data != nullptr);
							prim_storage.has_tangent = (tangent.data != nullptr);

							for (size_t v = 0; v < accessor_pos.count; ++v) {
								StaticMesh::VertexData vert{};
								vert.position = glm::vec4{ glm::make_vec3(&pos.data[v * pos.stride]), 1.f };
								if (normal.data) vert.normal = glm::normalize(glm::make_vec3(&normal.data[v * normal.stride]));
								if (uv0.data) vert.texcoord0 = glm::make_vec2(&uv0.data[v * uv0.stride]);
								if (uv1.data) vert.texcoord1 = glm::make_vec2(&uv1.data[v * uv1.stride]);
								if (tangent.data) vert.tangent = glm::make_vec4(&tangent.data[v * tangent.stride]);
								if (color.data) vert.color = glm::make_vec4(&color.data[v * color.stride]);

								mesh_storage->vertices.push_back(vert);
							}

							vertex_start += static_cast<uint32_t>(accessor_pos.count);
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
								process_indices<StaticMesh, uint32_t>(ptr, accessor.count, mesh_storage, index_start, vertex_start_copy);
								break;
							case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
								process_indices<StaticMesh, uint16_t>(ptr, accessor.count, mesh_storage, index_start, vertex_start_copy);
								break;
							case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
								process_indices<StaticMesh, uint8_t>(ptr, accessor.count, mesh_storage, index_start, vertex_start_copy);
								break;
							default:
								CORE_WARN("skip mesh for index type not supported");
								return;
							}
						}

						if (primitive.material >= 0) {
							prim_storage.material = loaded_materials[primitive.material];
						}

						mesh_storage->primitives.push_back(prim_storage);
					}

					auto const& root = FileSystem::s_content_root;

					std::string name = mesh.name.empty() ? ("SM_" + std::to_string(node.mesh)) : mesh.name;
					auto meta = mesh_storage->import(settings.path / name);
					if (meta.guid.is_valid()) {
						ret.assets.push_back(meta);
						ret.files.push_back((root / (name + ".bin")));
						mesh_guid = meta.guid;
					}
				}

				loaded_meshes[node.mesh] = { mesh_guid, is_skeletal };
			}

			if (mesh_guid.is_valid()) {
				if (is_skeletal) {
					yaml << YAML::Key << "skeletal_mesh" << YAML::Value;
					yaml << YAML::BeginMap;
					yaml << YAML::Key << "mesh" << YAML::Value << mesh_guid;
					if (node.skin > -1 && loaded_skeletons.size() > node.skin) {
						yaml << YAML::Key << "skeleton" << YAML::Value << loaded_skeletons[node.skin];
					}
					yaml << YAML::EndMap;
				}
				else {
					yaml << YAML::Key << "static_mesh" << YAML::Value;
					yaml << YAML::BeginMap;
					yaml << YAML::Key << "guid" << YAML::Value << mesh_guid;
					yaml << YAML::EndMap;
				}
			}
		}

		yaml << YAML::EndMap;

		if (node.children.size() > 0) {
			for (auto i : node.children) {
				load_node(settings, model.nodes[i], model, ret, loaded_materials, loaded_skeletons, loaded_meshes, yaml, current_id, my_id);
			}
		}
	}

	template<typename T>
	static void process_indices(const void* ptr, size_t count, std::shared_ptr<SkeletalMesh::Storage> const& mesh_storage, uint32_t& index_start, uint32_t vertex_start) {
		const T* buf = static_cast<const T*>(ptr);
		for (size_t i = 0; i < count; ++i) {
			mesh_storage->indices.push_back(buf[i] + vertex_start);
			++index_start;
		}
	}

	static bool rgb_to_rgba(std::vector<uint8_t> const& rgb, std::vector<uint8_t>& rgba, uint8_t alpha = 255) {
		if (rgb.size() % 3 != 0) {
			CORE_ERROR("rgb data size must be a multiple of 3");
			return false;
		}

		if (rgb.empty()) {
			rgba.clear();
			return true;
		}

		auto const pixel_count = rgb.size() / 3;
		rgba.resize(rgb.size() / 3 * 4);

		for (size_t i = 0; i < pixel_count; ++i) {
			auto const rgb_idx = i * 3;
			auto const rgba_idx = i * 4;

			rgba[rgba_idx + 0] = rgb[rgb_idx + 0];
			rgba[rgba_idx + 1] = rgb[rgb_idx + 1];
			rgba[rgba_idx + 2] = rgb[rgb_idx + 2];
			rgba[rgba_idx + 3] = alpha;
		}

		return true;
	}

	static void flip_vertically(tinygltf::Image& image) {
		// flip vertically
		int row_size = image.width * image.component;
		std::vector<uint8_t> temp_row(row_size);
		for (int y = 0; y < image.height / 2; ++y) {
			auto top = &image.image[y * row_size];
			auto bottom = &image.image[(image.height - 1 - y) * row_size];
			std::memcpy(temp_row.data(), top, row_size);
			std::memcpy(top, bottom, row_size);
			std::memcpy(bottom, temp_row.data(), row_size);
		}
	}

	static void import_textures(
		GltfImporterSettings const& settings,
		tinygltf::Model& model,
		ImportResult& ret,
		std::vector<Guid>& loaded_textures) {
		for (auto const& tex : model.textures) {
			if (tex.source < 0 || tex.source >= model.images.size()) {
				continue;
			}

			auto& image = model.images[tex.source];
			if (image.image.empty()) {
				continue;
			}

			//flip_vertically(image);

			uint8_t const* data_ptr = nullptr;
			std::vector<uint8_t> rgba_data;

			if (image.component == 3) {
				if (!rgb_to_rgba(image.image, rgba_data, 255)) {
					CORE_ERROR("failed to convert rgb to rgba for texture: {}", tex.source);
					continue;
				}
				data_ptr = rgba_data.data();
			}
			else if (image.component == 4) {
				data_ptr = image.image.data();
			}
			else {
				CORE_ERROR("unsupported number of component: {} for texture: {}", image.component, tex.source);
				continue;
			}

			auto const& root = FileSystem::s_content_root;

			std::string name = image.name.empty() ? ("T_" + std::to_string(tex.source)) : image.name;
			Filepath import_file = root / settings.path / (name + ".bin");

			AssetMeta meta{};
			meta.guid = Guid::generate();
			meta.type = "texture2d";
			meta.path = settings.path / name;

			// TODO
			// here we only use min filter as sampler mode
			// and only use wrap s as wrap mode
			SamplerMode sampler_mode = SamplerMode::Linear;
			WrapMode wrap_mode = WrapMode::Repeat;
			if (tex.sampler >= 0) {
				auto const& sampler = model.samplers[tex.sampler];
				switch (sampler.minFilter) {
				case TINYGLTF_TEXTURE_FILTER_NEAREST:
				case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
				case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
					sampler_mode = SamplerMode::Nearest;
					break;
				default:
					break;
				}

				switch (sampler.wrapS) {
				case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
					wrap_mode = WrapMode::ClampToEdge; break;
				case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
					wrap_mode = WrapMode::MirroredRepeat; break;
				default: break;
				}
			}

			meta.extra["sampler_mode"] = (int)sampler_mode;
			meta.extra["wrap_mode"] = (int)wrap_mode;
			meta.extra["hdr"] = false;

			if (!g_runtime_context.m_asset_manager->register_asset(meta, root)) {
				continue;
			}

			if (!bakery::compress_image_data(
				import_file,
				data_ptr,
				image.width,
				image.height)) {
				continue;
			}

			YAML::Emitter yaml;

			yaml << YAML::BeginMap;
			yaml << YAML::Key << "meta" << YAML::Value << meta;
			yaml << YAML::EndMap;

			if (!save_yaml(import_file.replace_extension(".yaml"), yaml)) {
				continue;
			}

			ret.assets.push_back(meta);
			ret.files.push_back(import_file);
			loaded_textures.push_back(meta.guid);
		}
	}

	static void handle_texture(
		std::shared_ptr<MaterialInstance> const& mi,
		tinygltf::Material& mat,
		std::vector<Guid> const& loaded_textures,
		std::string const& gltf_name,
		std::string const& shader_name,
		std::string const& uv_name) {
		int tex_coord = 0;
		if (mat.values.find(gltf_name) != mat.values.end()) {
			auto const& guid = loaded_textures[mat.values[gltf_name].TextureIndex()];
			mi->m_override_variables[shader_name].default_value.valid = true;
			mi->m_override_variables[shader_name].default_value.tex2D = g_runtime_context.m_asset_manager->get<Texture2D>(guid);
			tex_coord = mat.values[gltf_name].TextureTexCoord();
		}
		if (mat.additionalValues.find(gltf_name) != mat.additionalValues.end()) {
			auto const& guid = loaded_textures[mat.additionalValues[gltf_name].TextureIndex()];
			mi->m_override_variables[shader_name].default_value.valid = true;
			mi->m_override_variables[shader_name].default_value.tex2D = g_runtime_context.m_asset_manager->get<Texture2D>(guid);
			tex_coord = mat.additionalValues[gltf_name].TextureTexCoord();
		}

		if (!uv_name.empty()) {
			mi->m_override_variables[uv_name].default_value.valid = true;
			mi->m_override_variables[uv_name].default_value.ivec[0] = tex_coord;
		}
	}

	static void import_materials(
		GltfImporterSettings const& settings,
		tinygltf::Model& model,
		ImportResult& ret,
		std::vector<Guid> const& loaded_textures,
		std::vector<Guid>& loaded_materials) {
		int index = 0;
		for (auto& mat : model.materials) {
			std::string name = mat.name.empty() ? ("MI_" + std::to_string(index)) : mat.name;

			// TODO:
			// mat.doubleSided;

			// base material for material instance
			auto base = g_runtime_context.m_asset_manager->get<Material>(Guid::make("material/M_pbr"));

			// Check for KHR_materials_pbrSpecularGlossiness
			bool has_specular_glossiness = false;
			if (mat.extensions.find("KHR_materials_pbrSpecularGlossiness") != mat.extensions.end()) {
				has_specular_glossiness = true;
				base = g_runtime_context.m_asset_manager->get<Material>(Guid::make("material/M_pbr_sg"));
			}

			if (mat.doubleSided) {
				// Clone material to modify pipeline state
				auto new_mat = std::make_shared<Material>(base->m_pipeline_desc);
				new_mat->m_pipeline_desc.cull_mode = CullMode::None;
				new_mat->m_double_sided = true;
				new_mat->m_variables = base->m_variables;
				new_mat->m_pipeline = Pipeline::build(new_mat->m_pipeline_desc);

				// Register new material asset?
				// For now, let's keep it in memory or unique to this import?
				// MaterialInstance needs a shared_ptr<Material>.
				// If we don't register it, it won't be saved/loaded by GUID.
				// But we are creating a unique material per variance.
				// This is getting complex for a simple import.
				// Let's just modify the instance's material pointer to point to a new material.
				// But we need to save this new material to disk if we want persistence.

				// Alternative: Create M_pbr_double_sided, M_pbr_blend, etc.
				// Or dynamically create and save "M_pbr_variance_X"

				// For this task, let's dynamically create a new material asset relative to the import path.
				AssetMeta new_meta = base->m_meta;
				new_meta.guid = Guid::generate();
				new_meta.path = settings.path / (name + "_Mat");
				new_mat->m_meta = new_meta;

				g_runtime_context.m_asset_manager->register_asset(new_meta, FileSystem::s_content_root);
				// We need to save it too.
				new_mat->save();

				base = new_mat;
			}

			if (mat.alphaMode == "MASK") {
				if (base->m_alpha_mode != Material::AlphaMode::Mask) {
					// Clone if not already cloned
					if (base->m_meta.guid == Guid::make("material/M_pbr") || base->m_meta.guid == Guid::make("material/M_pbr_sg")) {
						auto new_mat = std::make_shared<Material>(base->m_pipeline_desc);
						new_mat->m_variables = base->m_variables;
						new_mat->m_pipeline_desc = base->m_pipeline_desc; // copy

						AssetMeta new_meta = base->m_meta;
						new_meta.guid = Guid::generate();
						new_meta.path = settings.path / (name + "_Mat");
						new_mat->m_meta = new_meta;

						g_runtime_context.m_asset_manager->register_asset(new_meta, FileSystem::s_content_root);
						base = new_mat;
					}

					base->m_alpha_mode = Material::AlphaMode::Mask;
					base->m_alpha_cutoff = (float)mat.alphaCutoff;
					// Mask usually doesn't need blending, just discard in shader.
				}
			}
			else if (mat.alphaMode == "BLEND") {
				if (base->m_alpha_mode != Material::AlphaMode::Blend) {
					if (base->m_meta.guid == Guid::make("material/M_pbr") || base->m_meta.guid == Guid::make("material/M_pbr_sg")) {
						auto new_mat = std::make_shared<Material>(base->m_pipeline_desc);
						new_mat->m_variables = base->m_variables;
						new_mat->m_pipeline_desc = base->m_pipeline_desc;

						AssetMeta new_meta = base->m_meta;
						new_meta.guid = Guid::generate();
						new_meta.path = settings.path / (name + "_Mat");
						new_mat->m_meta = new_meta;

						g_runtime_context.m_asset_manager->register_asset(new_meta, FileSystem::s_content_root);
						base = new_mat;
					}

					base->m_alpha_mode = Material::AlphaMode::Blend;
					base->m_pipeline_desc.blend = true;
					// Standard alpha blending
					base->m_pipeline_desc.src_blend_factor = BlendFactor::SrcAlpha;
					base->m_pipeline_desc.dst_blend_factor = BlendFactor::OneMinusSrcAlpha;
					base->m_pipeline = Pipeline::build(base->m_pipeline_desc);
				}
			}

			// Save the base material if we modified/created it
			if (base->m_meta.guid != Guid::make("material/M_pbr") && base->m_meta.guid != Guid::make("material/M_pbr_sg")) {
				base->save();
			}

			auto mi = MaterialInstance::create(settings.path / name, base);

			if (has_specular_glossiness) {
				auto const& ext = mat.extensions["KHR_materials_pbrSpecularGlossiness"];

				// Handle textures
				if (ext.Has("diffuseTexture")) {
					int index = ext.Get("diffuseTexture").Get("index").Get<int>();
					auto const& guid = loaded_textures[index];
					mi->m_override_variables["s_diffuse"].default_value.valid = true;
					mi->m_override_variables["s_diffuse"].default_value.tex2D = g_runtime_context.m_asset_manager->get<Texture2D>(guid);
				}
				if (ext.Has("specularGlossinessTexture")) {
					int index = ext.Get("specularGlossinessTexture").Get("index").Get<int>();
					auto const& guid = loaded_textures[index];
					mi->m_override_variables["s_specular_glossiness"].default_value.valid = true;
					mi->m_override_variables["s_specular_glossiness"].default_value.tex2D = g_runtime_context.m_asset_manager->get<Texture2D>(guid);
				}

				// Handle factors
				if (ext.Has("diffuseFactor")) {
					auto const& factor = ext.Get("diffuseFactor");
					mi->m_override_variables["u_diffuse_factor"].default_value.valid = true;
					for (int i = 0; i < 4; ++i) {
						mi->m_override_variables["u_diffuse_factor"].default_value.vec[i] = static_cast<float>(factor.Get(i).Get<double>());
					}
				}
				if (ext.Has("specularFactor")) {
					auto const& factor = ext.Get("specularFactor");
					mi->m_override_variables["u_specular_factor"].default_value.valid = true;
					for (int i = 0; i < 3; ++i) {
						mi->m_override_variables["u_specular_factor"].default_value.vec[i] = static_cast<float>(factor.Get(i).Get<double>());
					}
				}
				if (ext.Has("glossinessFactor")) {
					mi->m_override_variables["u_glossiness_factor"].default_value.valid = true;
					mi->m_override_variables["u_glossiness_factor"].default_value.vec[0] = static_cast<float>(ext.Get("glossinessFactor").Get<double>());
				}
			}
			else {
				// Standard PBR
				handle_texture(mi, mat, loaded_textures, "baseColorTexture", "s_base_color", "u_base_color_uv_set");
				handle_texture(mi, mat, loaded_textures, "metallicRoughnessTexture", "s_metallic_roughness", "u_metallic_roughness_uv_set");

				if (mat.values.find("baseColorFactor") != mat.values.end()) {
					mi->m_override_variables["u_base_color_factor"].default_value.valid = true;
					for (int i = 0; i < 4; ++i) {
						mi->m_override_variables["u_base_color_factor"].default_value.vec[i] = static_cast<float>(mat.values["baseColorFactor"].ColorFactor()[i]);
					}
				}
				if (mat.values.find("roughnessFactor") != mat.values.end()) {
					mi->m_override_variables["u_roughness_factor"].default_value.valid = true;
					mi->m_override_variables["u_roughness_factor"].default_value.vec[0] = static_cast<float>(mat.values["roughnessFactor"].Factor());
				}
				else if (mi->m_override_variables.find("s_metallic_roughness") != mi->m_override_variables.end()) {
					mi->m_override_variables["u_roughness_factor"].default_value.valid = true;
					mi->m_override_variables["u_roughness_factor"].default_value.vec[0] = 1.0f;
				}
				if (mat.values.find("metallicFactor") != mat.values.end()) {
					mi->m_override_variables["u_metallic_factor"].default_value.valid = true;
					mi->m_override_variables["u_metallic_factor"].default_value.vec[0] = static_cast<float>(mat.values["metallicFactor"].Factor());
				}
				else if (mi->m_override_variables.find("s_metallic_roughness") != mi->m_override_variables.end()) {
					mi->m_override_variables["u_metallic_factor"].default_value.valid = true;
					mi->m_override_variables["u_metallic_factor"].default_value.vec[0] = 1.0f;
				}
			}

			handle_texture(mi, mat, loaded_textures, "normalTexture", "s_normal", "u_normal_uv_set");
			handle_texture(mi, mat, loaded_textures, "emissiveTexture", "s_emissive", "u_emissive_uv_set");
			handle_texture(mi, mat, loaded_textures, "occlusionTexture", "s_occlusion", "u_occlusion_uv_set");

			if (mat.alphaMode == "MASK") {
				mi->m_override_variables["u_alpha_cutoff"].default_value.valid = true;
				mi->m_override_variables["u_alpha_cutoff"].default_value.vec[0] = (float)mat.alphaCutoff;
			}

			mi->save();
			loaded_materials.push_back(mi->m_meta.guid);
			index += 1;
		}
	}

	static Guid import_skeleton(
		GltfImporterSettings const& settings,
		tinygltf::Model const& model,
		tinygltf::Skin const& skin,
		ImportResult& ret) {

		Skeleton skeleton;
		std::vector<glm::mat4> inverse_bind_matrices;

		if (skin.inverseBindMatrices > -1) {
			auto const& accessor = model.accessors[skin.inverseBindMatrices];
			auto const& view = model.bufferViews[accessor.bufferView];
			auto const& buffer = model.buffers[view.buffer];
			const float* data = reinterpret_cast<const float*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
			for (size_t i = 0; i < accessor.count; ++i) {
				inverse_bind_matrices.push_back(glm::make_mat4(&data[i * 16]));
			}
		}

		std::vector<int> joints = skin.joints;
		if (joints.empty()) {
			// If no joints specified, maybe all nodes are joints?
			// But usually skin has joints.
			return Guid();
		}

		// Map node index to bone index
		std::unordered_map<int, int> node_to_bone_index;
		for (size_t i = 0; i < joints.size(); ++i) {
			node_to_bone_index[joints[i]] = static_cast<int>(i);
		}

		for (size_t i = 0; i < joints.size(); ++i) {
			int node_idx = joints[i];
			auto const& node = model.nodes[node_idx];

			Bone bone;
			bone.name = node.name;
			bone.id = static_cast<int>(i); // Bone index in skeleton
			bone.node_index = node_idx;
			bone.local_bind_transform = get_transform(node);

			// Find parent
			// We need to find if any parent of this node is also in the skeleton
			// Note: glTF nodes have children, but not parent pointer.
			// But we can iterate over all joints to find which one has this node as child.

			// Optimization: Since we are iterating all joints, for the current joint, we can check its children.
			// If a child is also a joint, we set the child's parent_id to current joint.
			// But here we are processing 'bone'. We need to find ITS parent.

			// Let's rely on a separate pass or just search.
			// Since we process bones in order of 'skin.joints', usually topological sort is not guaranteed but common.
			// But we need parent_id.

			// Search who is parent of node_idx
			int parent_node_idx = -1;
			bool found_parent = false;

			// Iterate all nodes to find who lists node_idx as child
			// This is slow O(N*M). But N (nodes) is small usually.
			// Better: build node->parent map once for the whole model.
			// But here, let's just search within the joints?
			// The parent MUST be in the skeleton for it to be a valid bone parent.

			for (size_t j = 0; j < joints.size(); ++j) {
				if (i == j) continue;
				int other_node_idx = joints[j];
				auto const& other_node = model.nodes[other_node_idx];
				for (int child : other_node.children) {
					if (child == node_idx) {
						parent_node_idx = other_node_idx;
						found_parent = true;
						break;
					}
				}
				if (found_parent) break;
			}

			if (found_parent) {
				if (node_to_bone_index.find(parent_node_idx) != node_to_bone_index.end()) {
					bone.parent_id = node_to_bone_index[parent_node_idx];
				}
			}

			if (i < inverse_bind_matrices.size()) {
				bone.offset_matrix = inverse_bind_matrices[i];
			}

			skeleton.bones.push_back(bone);
		}

		// Save skeleton
		auto const& root = FileSystem::s_content_root;
		std::string name = skin.name.empty() ? ("SK_" + std::to_string(ret.assets.size())) : skin.name;
		// Ensure unique name if multiple skins

		AssetMeta meta{};
		meta.guid = Guid::generate();
		meta.type = "skeleton";
		meta.path = settings.path / name;

		if (!g_runtime_context.m_asset_manager->register_asset(meta, root)) {
			return {};
		}

		YAML::Emitter yaml;
		yaml << YAML::BeginMap;
		yaml << YAML::Key << "bones" << YAML::Value << YAML::BeginSeq;
		for (auto const& bone : skeleton.bones) {
			yaml << YAML::BeginMap;
			yaml << YAML::Key << "name" << YAML::Value << bone.name;
			yaml << YAML::Key << "id" << YAML::Value << bone.id;
			yaml << YAML::Key << "node_index" << YAML::Value << bone.node_index;
			yaml << YAML::Key << "parent_id" << YAML::Value << bone.parent_id;
			yaml << YAML::Key << "offset_matrix" << YAML::Value << bone.offset_matrix;
			yaml << YAML::Key << "local_bind_transform" << YAML::Value << bone.local_bind_transform;
			yaml << YAML::EndMap;
		}
		yaml << YAML::EndSeq;

		yaml << YAML::Key << "meta" << YAML::Value << meta;
		yaml << YAML::EndMap;

		Filepath file = root / meta.path;
		BinaryFile bf;
		bf.set_yaml(yaml.c_str());
		if (bf.save(file.concat(".bin"))) {
			save_yaml(file.replace_extension(".yaml"), yaml);
			ret.assets.push_back(meta);
			ret.files.push_back(file);
			return meta.guid;
		}
		return {};
	}

	static void import_animation(
		GltfImporterSettings const& settings,
		tinygltf::Model const& model,
		tinygltf::Animation const& anim,
		ImportResult& ret) {

		Animation animation;
		animation.name = anim.name;
		animation.ticks_per_second = 1.0f; // GLTF uses seconds, so 1 tick = 1 second

		for (auto const& channel : anim.channels) {
			if (channel.target_node == -1) continue;

			AnimationChannel anim_channel;
			anim_channel.bone_id = channel.target_node; // This is node index, might need mapping to bone index later
			// But wait, AnimationChannel stores 'bone_name'.
			auto const& node = model.nodes[channel.target_node];
			anim_channel.bone_name = node.name;

			auto const& sampler = anim.samplers[channel.sampler];
			auto const& input_accessor = model.accessors[sampler.input];
			auto const& output_accessor = model.accessors[sampler.output];

			std::vector<float> times;
			{
				auto const& view = model.bufferViews[input_accessor.bufferView];
				auto const& buffer = model.buffers[view.buffer];
				const uint8_t* data = &buffer.data[input_accessor.byteOffset + view.byteOffset];
				int stride = input_accessor.ByteStride(view) ? input_accessor.ByteStride(view) : sizeof(float);

				for (size_t i = 0; i < input_accessor.count; ++i) {
					const float* val = reinterpret_cast<const float*>(data + i * stride);
					times.push_back(*val);
					animation.duration = std::max(animation.duration, *val);
				}
			}

			auto const& view = model.bufferViews[output_accessor.bufferView];
			auto const& buffer = model.buffers[view.buffer];
			const uint8_t* data = &buffer.data[output_accessor.byteOffset + view.byteOffset];

			bool is_cubic = sampler.interpolation == "CUBICSPLINE";

			if (output_accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
				CORE_WARN("Animation channel {} has non-float component type. Skipping.", channel.target_path);
				continue;
			}

			if (channel.target_path == "translation") {
				size_t element_size = 3 * sizeof(float);
				size_t stride = view.byteStride ? view.byteStride : (is_cubic ? 3 : 1) * element_size;
				for (size_t i = 0; i < output_accessor.count; ++i) {
					PositionKeyframe key;
					key.time = times[i];
					const float* val_ptr = reinterpret_cast<const float*>(data + i * stride);
					if (is_cubic) val_ptr += 3; // Skip in-tangent (vec3)
					key.value = glm::make_vec3(val_ptr);
					anim_channel.position_keys.push_back(key);
				}
			}
			else if (channel.target_path == "rotation") {
				size_t element_size = 4 * sizeof(float);
				size_t stride = view.byteStride ? view.byteStride : (is_cubic ? 3 : 1) * element_size;
				for (size_t i = 0; i < output_accessor.count; ++i) {
					RotationKeyframe key;
					key.time = times[i];
					const float* val_ptr = reinterpret_cast<const float*>(data + i * stride);
					if (is_cubic) val_ptr += 4; // Skip in-tangent (vec4)
					key.value = glm::normalize(glm::make_quat(val_ptr)); // Ensure normalized
					anim_channel.rotation_keys.push_back(key);
				}
			}
			else if (channel.target_path == "scale") {
				size_t element_size = 3 * sizeof(float);
				size_t stride = view.byteStride ? view.byteStride : (is_cubic ? 3 : 1) * element_size;
				for (size_t i = 0; i < output_accessor.count; ++i) {
					ScaleKeyframe key;
					key.time = times[i];
					const float* val_ptr = reinterpret_cast<const float*>(data + i * stride);
					if (is_cubic) val_ptr += 3; // Skip in-tangent (vec3)
					key.value = glm::make_vec3(val_ptr);
					anim_channel.scale_keys.push_back(key);
				}
			}

			bool found = false;
			for (auto& existing : animation.channels) {
				if (existing.bone_id == anim_channel.bone_id) {
					if (!anim_channel.position_keys.empty()) existing.position_keys = anim_channel.position_keys;
					if (!anim_channel.rotation_keys.empty()) existing.rotation_keys = anim_channel.rotation_keys;
					if (!anim_channel.scale_keys.empty()) existing.scale_keys = anim_channel.scale_keys;
					found = true;
					break;
				}
			}
			if (!found) {
				animation.channels.push_back(anim_channel);
			}
		}

		// Save animation
		auto const& root = FileSystem::s_content_root;
		std::string name = anim.name.empty() ? ("AN_" + std::to_string(ret.assets.size())) : anim.name;

		AssetMeta meta{};
		meta.guid = Guid::generate();
		meta.type = "animation";
		meta.path = settings.path / name;

		if (!g_runtime_context.m_asset_manager->register_asset(meta, root)) {
			return;
		}

		size_t data_size = 0;
		for (auto const& ch : animation.channels) {
			data_size += ch.position_keys.size() * sizeof(PositionKeyframe);
			data_size += ch.rotation_keys.size() * sizeof(RotationKeyframe);
			data_size += ch.scale_keys.size() * sizeof(ScaleKeyframe);
		}

		BinaryFile bf;
		bf.reserve(data_size);

		YAML::Emitter yaml;
		yaml << YAML::BeginMap;
		yaml << YAML::Key << "name" << YAML::Value << animation.name;
		yaml << YAML::Key << "duration" << YAML::Value << animation.duration;
		yaml << YAML::Key << "ticks_per_second" << YAML::Value << animation.ticks_per_second;
		yaml << YAML::Key << "channels" << YAML::Value << YAML::BeginSeq;

		size_t current_offset = 0;
		for (auto const& ch : animation.channels) {
			yaml << YAML::BeginMap;
			yaml << YAML::Key << "bone_name" << YAML::Value << ch.bone_name;
			yaml << YAML::Key << "bone_id" << YAML::Value << ch.bone_id;

			size_t pos_size = ch.position_keys.size() * sizeof(PositionKeyframe);
			size_t rot_size = ch.rotation_keys.size() * sizeof(RotationKeyframe);
			size_t scl_size = ch.scale_keys.size() * sizeof(ScaleKeyframe);

			yaml << YAML::Key << "pos_count" << YAML::Value << ch.position_keys.size();
			yaml << YAML::Key << "rot_count" << YAML::Value << ch.rotation_keys.size();
			yaml << YAML::Key << "scl_count" << YAML::Value << ch.scale_keys.size();

			yaml << YAML::Key << "pos_offset" << YAML::Value << current_offset;
			if (pos_size > 0) {
				bf.set_data(ch.position_keys.data(), pos_size, current_offset);
				current_offset += pos_size;
			}

			yaml << YAML::Key << "rot_offset" << YAML::Value << current_offset;
			if (rot_size > 0) {
				bf.set_data(ch.rotation_keys.data(), rot_size, current_offset);
				current_offset += rot_size;
			}

			yaml << YAML::Key << "scl_offset" << YAML::Value << current_offset;
			if (scl_size > 0) {
				bf.set_data(ch.scale_keys.data(), scl_size, current_offset);
				current_offset += scl_size;
			}

			yaml << YAML::EndMap;
		}
		yaml << YAML::EndSeq;

		yaml << YAML::Key << "meta" << YAML::Value << meta;
		yaml << YAML::EndMap;

		bf.set_yaml(yaml.c_str());
		Filepath file = root / meta.path;
		if (bf.save(file.concat(".bin"))) {
			save_yaml(file.replace_extension(".yaml"), yaml);
			ret.assets.push_back(meta);
			ret.files.push_back(file);
		}
	}

	ImportResult GltfImporter::import(GltfImporterSettings const& settings) {
		ImportResult ret{};

		if (!can_import(settings.file)) {
			CORE_WARN("{0} is not a common gltf file", settings.file.generic_string());
			return ret;
		}

		tinygltf::Model model{};
		tinygltf::TinyGLTF loader{};
		std::string err;
		std::string warn;

		stbi_set_flip_vertically_on_load(true);
		bool result = loader.LoadBinaryFromFile(&model, &err, &warn, settings.file.string());

		if (!err.empty()) {
			CORE_ERROR("failed to load static mesh: {0}", settings.file.generic_string());
			CORE_ERROR("TinyGLTF: {0}", err);
			return ret;
		}
		if (!warn.empty()) {
			CORE_WARN("TinyGLTF: {0}", warn);
		}
		if (!result) {
			CORE_ERROR("failed to load static mesh: {0}", settings.file.generic_string());
			return ret;
		}

		std::vector<Guid> loaded_textures;
		import_textures(settings, model, ret, loaded_textures);
		std::vector<Guid> loaded_materials;
		import_materials(settings, model, ret, loaded_textures, loaded_materials);

		// load skins (skeletons)
		std::vector<Guid> loaded_skeletons;
		for (auto const& skin : model.skins) {
			auto guid = import_skeleton(settings, model, skin, ret);
			if (guid.is_valid()) {
				loaded_skeletons.push_back(guid);
			}
			else {
				loaded_skeletons.push_back(Guid());
			}
		}

		// load default scene.
		auto const& default_scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

		YAML::Emitter yaml;
		yaml << YAML::BeginMap;
		yaml << YAML::Key << "entities" << YAML::Value << YAML::BeginSeq;

		int current_id = 1;
		std::map<int, std::pair<Guid, bool>> loaded_meshes;

		// nodes will be loaded as 'Entity'
		// meshes will be loaded as 'StaticMesh'
		// primitives will be loaded as 'StaticMesh::Primitive'
		for (auto i : default_scene.nodes) {
			auto const& node = model.nodes[i];
			load_node(settings, node, model, ret, loaded_materials, loaded_skeletons, loaded_meshes, yaml, current_id, -1);
		}

		yaml << YAML::EndSeq;

		std::string prefab_name = settings.file.stem().string();
		auto prefab_path = settings.path / (prefab_name + ".prefab");

		AssetMeta prefab_meta{};
		prefab_meta.guid = Guid::generate();
		prefab_meta.type = "prefab";
		prefab_meta.path = prefab_path;

		yaml << YAML::Key << "meta" << YAML::Value << prefab_meta;
		yaml << YAML::EndMap;

		{
			auto physical_path = FileSystem::s_content_root / prefab_path;
			physical_path += ".yaml";

			std::ofstream fout(physical_path);
			fout << yaml.c_str();
			fout.close();

			if (g_runtime_context.m_asset_manager->register_asset(prefab_meta, FileSystem::s_content_root)) {
				ret.assets.push_back(prefab_meta);
				ret.files.push_back(physical_path);
			}
		}

		// load animations
		for (auto const& anim : model.animations) {
			import_animation(settings, model, anim, ret);
		}

		ret.success = true;
		return ret;
	}

}
