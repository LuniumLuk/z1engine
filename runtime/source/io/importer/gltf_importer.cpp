#include "pch.h"
#include "io/importer/gltf_importer.h"
#include "io/loader/mesh_storage.h"
#include "scene/entity.h"
#include "scene/component/mesh.h"
#include "render/mesh.h"
#include "render/material.h"
#include "util/yaml.h"

#include "bakery.h"
#include "tinygltf/tiny_gltf.h"
#include "glm/gtc/type_ptr.hpp"

namespace z1::io {

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

	template<typename T>
	static void process_indices(const void* ptr, size_t count, std::shared_ptr<StaticMesh::Storage> const& mesh_storage, uint32_t& index_start, uint32_t vertex_start) {
		const T* buf = static_cast<const T*>(ptr);
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
		ImportResult& ret) {

		auto const& name = node.name;
		/*auto entity = scene->create_entity(name);
		auto& transform_comp = entity->get_component<TransformComponent>();
		transform_comp.m_parent = parent;
		transform_comp.set_local_transform(get_transform(node));*/

		if (node.children.size() > 0) {
			for (auto i : node.children) {
				load_node(settings, model.nodes[i], model, ret);
			}
		}

		auto mesh_storage = std::make_shared<StaticMesh::Storage>();
		mesh_storage->bound_min = glm::vec3{ FLT_MAX };
		mesh_storage->bound_max = glm::vec3{ FLT_MIN };

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

			mesh_storage->vertices.reserve(mesh_vertex_count);
			mesh_storage->indices.reserve(mesh_index_count);

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
					mesh_storage->bound_min = glm::min(mesh_storage->bound_min, prim_storage.bound_min);
					mesh_storage->bound_max = glm::max(mesh_storage->bound_max, prim_storage.bound_max);

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

						mesh_storage->vertices.push_back(vert);
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

				mesh_storage->primitives.push_back(prim_storage);
			}

			auto const& root = FileSystem::s_content_root;

			std::string name = mesh.name.empty() ? ("SM_" + std::to_string(node.mesh)) : mesh.name;
			Filepath import_file = root / settings.path / (name + ".bin");
			Filepath import_meta = import_file;
			import_meta += ".meta.yaml";

			AssetMetaData meta{};
			meta.guid = Guid::generate();
			meta.type = "static mesh";
			meta.path = settings.path / name;

			g_runtime_context.m_asset_manager->register_asset(meta, root);

			if (io::save_static_mesh_storage(import_file, mesh_storage)) {
				meta.save(import_meta);
				ret.assets.push_back(meta);
				ret.files.push_back(import_file);
			}

			//auto static_mesh = std::make_shared<StaticMesh>(mesh_storage);
			//entity->add_component<StaticMeshComponent>(static_mesh);
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

	static void import_textures(GltfImporterSettings const& settings, tinygltf::Model& model, ImportResult& ret) {
		for (auto const& tex : model.textures) {
			if (tex.source < 0 || tex.source >= model.images.size()) {
				continue;
			}

			auto& image = model.images[tex.source];
			if (image.image.empty()) {
				continue;
			}

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
			Filepath import_meta = import_file;
			import_meta += ".meta.yaml";

			AssetMetaData meta{};
			meta.guid = Guid::generate();
			meta.type = "image";
			meta.path = settings.path / name;

			auto const& sampler = model.samplers[tex.sampler];
			// TODO
			// here we only use min filter as sampler mode
			// and only use wrap s as wrap mode
			SamplerMode sampler_mode = SamplerMode::Linear;
			switch (sampler.minFilter) {
			case TINYGLTF_TEXTURE_FILTER_NEAREST:
			case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
			case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
				sampler_mode = SamplerMode::Nearest; break;
			default: break;
			}

			WrapMode wrap_mode = WrapMode::Repeat;
			switch (sampler.wrapS) {
			case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
				wrap_mode = WrapMode::ClampToEdge; break;
			case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
				wrap_mode = WrapMode::MirroredRepeat; break;
			default: break;
			}

			meta.extra["sampler_mode"] = (int)sampler_mode;
			meta.extra["wrap_mode"] = (int)wrap_mode;
			meta.extra["hdr"] = false;

			g_runtime_context.m_asset_manager->register_asset(meta, root);

			try {
				std::filesystem::create_directories(import_file.parent_path());
				bakery::compress_image_data(
					import_file,
					data_ptr,
					image.width,
					image.height
				);
				meta.save(import_meta);
			}
			catch (std::exception const& e) {
				CORE_ERROR("failed to compress image {}: {}", name, e.what());
				continue;
			}

			ret.assets.push_back(meta);
			ret.files.push_back(import_file);
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

		import_textures(settings, model, ret);

		// load default scene.
		auto const& default_scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

		// nodes will be loaded as 'Entity'
		// meshes will be loaded as 'StaticMesh'
		// primitives will be loaded as 'StaticMesh::Primitive'
		for (auto i : default_scene.nodes) {
			auto const& node = model.nodes[i];
			load_node(settings, node, model, ret);
		}

		ret.success = true;
		return ret;

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

	//void Model::loadMaterials(tinygltf::Model& model) {
	//	for (auto& mat : model.materials) {
	//		Material material{};
	//		material.doubleSided = mat.doubleSided;
	//		if (mat.normalTexture.extensions.find("KHR_texture_transform") != mat.normalTexture.extensions.end()) {
	//			std::cout << "Found KHR_texture_transform\n";
	//			auto ext = mat.normalTexture.extensions.find("KHR_texture_transform");
	//			if (ext->second.Has("offset")) {
	//				auto const& index = ext->second.Get("offset");
	//				for (uint32_t i = 0; i < index.ArrayLen(); i++) {
	//					auto const& val = index.Get(i);
	//					material.textureTransform.offset[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
	//				}
	//				std::cout << "- offset: " << material.textureTransform.offset[0] << ", " << material.textureTransform.offset[1] << "\n";
	//			}
	//			if (ext->second.Has("rotation")) {
	//				auto const& index = ext->second.Get("rotation");
	//				material.textureTransform.rotation = index.IsNumber() ? (float)index.Get<double>() : (float)index.Get<int>();
	//			}
	//			if (ext->second.Has("scale")) {
	//				auto const& index = ext->second.Get("scale");
	//				for (uint32_t i = 0; i < index.ArrayLen(); i++) {
	//					auto const& val = index.Get(i);
	//					material.textureTransform.scale[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
	//				}
	//				std::cout << "- scale: " << material.textureTransform.scale[0] << ", " << material.textureTransform.scale[1] << "\n";
	//			}
	//		}

	//		if (mat.values.find("baseColorTexture") != mat.values.end()) {
	//			material.baseColorTexture = &mTextures[mat.values["baseColorTexture"].TextureIndex()];
	//			material.texCoordSets.baseColor = mat.values["baseColorTexture"].TextureTexCoord();
	//		}
	//		if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
	//			material.metallicRoughnessTexture = &mTextures[mat.values["metallicRoughnessTexture"].TextureIndex()];
	//			material.texCoordSets.metallicRoughness = mat.values["metallicRoughnessTexture"].TextureTexCoord();
	//		}
	//		if (mat.values.find("roughnessFactor") != mat.values.end()) {
	//			material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
	//		}
	//		if (mat.values.find("metallicFactor") != mat.values.end()) {
	//			material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
	//		}
	//		if (mat.values.find("baseColorFactor") != mat.values.end()) {
	//			material.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
	//		}
	//		if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
	//			material.normalTexture = &mTextures[mat.additionalValues["normalTexture"].TextureIndex()];
	//			material.texCoordSets.normal = mat.additionalValues["normalTexture"].TextureTexCoord();
	//		}
	//		if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
	//			material.emissiveTexture = &mTextures[mat.additionalValues["emissiveTexture"].TextureIndex()];
	//			material.texCoordSets.emissive = mat.additionalValues["emissiveTexture"].TextureTexCoord();
	//		}
	//		if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
	//			material.occlusionTexture = &mTextures[mat.additionalValues["occlusionTexture"].TextureIndex()];
	//			material.texCoordSets.occlusion = mat.additionalValues["occlusionTexture"].TextureTexCoord();
	//		}
	//		if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
	//			tinygltf::Parameter param = mat.additionalValues["alphaMode"];
	//			if (param.string_value == "BLEND") {
	//				material.alphaMode = Material::AlphaMode::Blend;
	//			}
	//			if (param.string_value == "MASK") {
	//				material.alphaCutoff = 0.5f;
	//				material.alphaMode = Material::AlphaMode::Mask;
	//			}
	//		}
	//		if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
	//			material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
	//		}
	//		if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
	//			material.emissiveFactor = glm::vec4(glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
	//		}

	//		if (mat.extensions.find("KHR_materials_pbrSpecularGlossiness") != mat.extensions.end()) {
	//			std::cerr << "Currently not support [KHR_materials_pbrSpecularGlossiness] extension\n";
	//			auto ext = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
	//			if (ext->second.Has("specularGlossinessTexture")) {
	//				auto const& index = ext->second.Get("specularGlossinessTexture").Get("index");
	//				material.pbrSpecularGlossiness.specularGlossinessTexture = &mTextures[index.Get<int>()];
	//				auto const& texCoordSet = ext->second.Get("specularGlossinessTexture").Get("texCoord");
	//				material.texCoordSets.specularGlossiness = texCoordSet.Get<int>();
	//			}
	//			if (ext->second.Has("diffuseTexture")) {
	//				auto const& index = ext->second.Get("diffuseTexture").Get("index");
	//				material.pbrSpecularGlossiness.diffuseTexture = &mTextures[index.Get<int>()];
	//			}
	//			if (ext->second.Has("diffuseFactor")) {
	//				auto const& factor = ext->second.Get("diffuseFactor");
	//				for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
	//					auto const& val = factor.Get(i);
	//					material.pbrSpecularGlossiness.diffuseFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
	//				}
	//			}
	//			if (ext->second.Has("specularFactor")) {
	//				auto const& factor = ext->second.Get("specularFactor");
	//				for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
	//					auto const& val = factor.Get(i);
	//					material.pbrSpecularGlossiness.specularFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
	//				}
	//			}
	//		}

	//		if (mat.extensions.find("KHR_materials_transmission") != mat.extensions.end()) {
	//			auto ext = mat.extensions.find("KHR_materials_transmission");
	//			if (ext->second.Has("transmissionTexture")) {
	//				auto const& index = ext->second.Get("transmissionTexture").Get("index");
	//				material.transmission.transmissionTexture = &mTextures[index.Get<int>()];
	//			}
	//			if (ext->second.Has("transmissionFactor")) {
	//				auto const& factor = ext->second.Get("transmissionFactor");
	//				material.transmission.transmissionFactor = static_cast<float>(factor.GetNumberAsDouble());
	//			}
	//		}

	//		if (mat.extensions.find("KHR_materials_ior") != mat.extensions.end()) {
	//			auto ext = mat.extensions.find("KHR_materials_ior");
	//			if (ext->second.Has("ior")) {
	//				auto const& factor = ext->second.Get("ior");
	//				material.indexOfRefraction.ior = static_cast<float>(factor.GetNumberAsDouble());
	//			}
	//		}

	//		if (mat.extensions.find("KHR_materials_specular") != mat.extensions.end()) {
	//			auto ext = mat.extensions.find("KHR_materials_specular");
	//			if (ext->second.Has("specularFactor")) {
	//				auto const& factor = ext->second.Get("specularFactor");
	//				material.specular.specularFactor = static_cast<float>(factor.GetNumberAsDouble());
	//			}
	//			if (ext->second.Has("specularTexture")) {
	//				auto const& index = ext->second.Get("specularTexture").Get("index");
	//				material.specular.specularTexture = &mTextures[index.Get<int>()];
	//			}
	//			if (ext->second.Has("specularMask")) {
	//				auto const& factor = ext->second.Get("specularFactor");
	//				material.specular.specularMask = static_cast<float>(factor.GetNumberAsDouble());
	//			}
	//			if (ext->second.Has("specularColorFactor")) {
	//				std::cerr << "Currently not support [specularColorFactor] in KHR_materials_specular\n";
	//			}
	//			if (ext->second.Has("specularColorTexture")) {
	//				std::cerr << "Currently not support [specularColorTexture] in KHR_materials_specular\n";
	//			}
	//		}

	//		if (mat.extensions.find("KHR_materials_volume") != mat.extensions.end()) {
	//			auto ext = mat.extensions.find("KHR_materials_volume");
	//			if (ext->second.Has("thicknessFactor")) {
	//				auto const& factor = ext->second.Get("thicknessFactor");
	//				material.volume.thicknessFactor = static_cast<float>(factor.GetNumberAsDouble());
	//			}
	//			if (ext->second.Has("thicknessTexture")) {
	//				auto const& index = ext->second.Get("thicknessTexture").Get("index");
	//				material.volume.thicknessTexture = &mTextures[index.Get<int>()];
	//			}
	//			if (ext->second.Has("attenuationDistance")) {
	//				auto const& factor = ext->second.Get("attenuationDistance");
	//				material.volume.attenuationDistance = static_cast<float>(factor.GetNumberAsDouble());
	//			}
	//			if (ext->second.Has("attenuationColor")) {
	//				auto const& factor = ext->second.Get("attenuationColor");
	//				for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
	//					auto const& val = factor.Get(i);
	//					material.volume.attenuationColor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
	//				}
	//			}
	//		}

	//		if (mat.extensions.find("DD_disney") != mat.extensions.end()) {
	//			auto ext = mat.extensions.find("DD_disney");
	//			if (ext->second.Has("subsurface")) {
	//				auto const& factor = ext->second.Get("subsurface");
	//				material.disneyBRDF.subsurface = static_cast<float>(factor.GetNumberAsDouble());
	//			}
	//			if (ext->second.Has("subsurfaceColor")) {
	//				auto const& factor = ext->second.Get("subsurfaceColor");
	//				for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
	//					auto const& val = factor.Get(i);
	//					material.disneyBRDF.subsurfaceColor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
	//				}
	//			}
	//		}

	//		if (mat.extensions.find("DD_detail_normal") != mat.extensions.end()) {
	//			auto ext = mat.extensions.find("DD_detail_normal");
	//			if (ext->second.Has("detailNormalTexture")) {
	//				auto const& index = ext->second.Get("detailNormalTexture").Get("index");
	//				material.detailNormal.detailNormalTexture = &mTextures[index.Get<int>()];
	//			}
	//			if (ext->second.Has("detailNormalScale")) {
	//				auto const& factor = ext->second.Get("detailNormalScale");
	//				material.detailNormal.detailNormalScale = static_cast<float>(factor.GetNumberAsDouble());
	//			}
	//			if (ext->second.Has("detailNormalWeight")) {
	//				auto const& factor = ext->second.Get("detailNormalWeight");
	//				material.detailNormal.detailNormalWeight = static_cast<float>(factor.GetNumberAsDouble());
	//			}
	//		}

	//		if (mat.extensions.find("DD_cavity") != mat.extensions.end()) {
	//			auto ext = mat.extensions.find("DD_cavity");
	//			if (ext->second.Has("cavityTexture")) {
	//				auto const& index = ext->second.Get("cavityTexture").Get("index");
	//				material.cavity.cavityTexture = &mTextures[index.Get<int>()];
	//			}
	//		}

	//		if (mat.extensions.find("DD_mask") != mat.extensions.end()) {
	//			auto ext = mat.extensions.find("DD_mask");
	//			if (ext->second.Has("mask01")) {
	//				auto const& index = ext->second.Get("mask01").Get("index");
	//				material.mask.mask01 = &mTextures[index.Get<int>()];
	//			}
	//			if (ext->second.Has("mask02")) {
	//				auto const& index = ext->second.Get("mask02").Get("index");
	//				material.mask.mask02 = &mTextures[index.Get<int>()];
	//			}
	//			if (ext->second.Has("mask03")) {
	//				auto const& index = ext->second.Get("mask03").Get("index");
	//				material.mask.mask03 = &mTextures[index.Get<int>()];
	//			}
	//			if (ext->second.Has("mask04")) {
	//				auto const& index = ext->second.Get("mask04").Get("index");
	//				material.mask.mask04 = &mTextures[index.Get<int>()];
	//			}
	//		}

	//		material.index = mMaterials.size();
	//		mMaterials.push_back(material);
	//	}
	//	// Default material for mesh with no material.
	//	auto material = Material{};
	//	material.index = mMaterials.size();
	//	mMaterials.push_back(material);
	//}


}
