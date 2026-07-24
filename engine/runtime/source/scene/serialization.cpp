#include "pch.h"
#include "scene/serialization.h"
#include "asset/asset_manager.h"
#include "asset/mesh.h"
#include "asset/texture.h"
#include "animation/animation.h"
#include "animation/skeleton.h"
#include "core/guid.h"
#include "core/core.h"
#include <glm/glm.hpp>

namespace z1 {

	std::string field_to_yaml_key(std::string const& field_name) {
		std::string result;
		size_t start = 0;
		if (field_name.size() > 2 && field_name[0] == 'm' && field_name[1] == '_') {
			start = 2;
		}
		for (size_t i = start; i < field_name.size(); ++i) {
			char c = field_name[i];
			if (c >= 'A' && c <= 'Z') {
				if (!result.empty() && result.back() != '_') {
					result += '_';
				}
				result += (char)(c + ('a' - 'A'));
			} else {
				result += c;
			}
		}
		return result;
	}

	static void serialize_value(YAML::Emitter& yaml, void* ptr, std::type_info const& type,
		FieldInfo const& field, const EnumInfo* enum_info) {

		if (field.custom_getter) {
			ptr = field.custom_getter(ptr);
		}

		if (field.is_guid) {
			auto* guid = static_cast<Guid*>(ptr);
			if (guid->is_valid()) {
				yaml << guid->value;
			} else {
				yaml << YAML::Null;
			}
			return;
		}

		if (field.is_asset_ref) {
			// Asset refs are stored as shared_ptr<AssetType>
			// Use asset_ref_to_guid_string which accounts for vtable offset
			std::string guid = asset_ref_to_guid_string(ptr);
			if (!guid.empty()) {
				yaml << guid;
			} else {
				yaml << YAML::Null;
			}
			return;
		}

		if (enum_info) {
			int value = *reinterpret_cast<int*>(ptr);
			std::string name = std::to_string(value);
			for (auto const& item : enum_info->items) {
				if (item.value == value) {
					name = item.name;
					break;
				}
			}
			yaml << name;
			return;
		}

		if (type == typeid(bool)) {
			yaml << *static_cast<bool*>(ptr);
		} else if (type == typeid(int)) {
			yaml << *static_cast<int*>(ptr);
		} else if (type == typeid(uint32_t)) {
			yaml << *static_cast<uint32_t*>(ptr);
		} else if (type == typeid(uint8_t)) {
			yaml << (int)*static_cast<uint8_t*>(ptr);
		} else if (type == typeid(int8_t)) {
			yaml << (int)*static_cast<int8_t*>(ptr);
		} else if (type == typeid(float)) {
			yaml << *static_cast<float*>(ptr);
		} else if (type == typeid(double)) {
			yaml << *static_cast<double*>(ptr);
		} else if (type == typeid(std::string)) {
			yaml << *static_cast<std::string*>(ptr);
		} else if (type == typeid(glm::vec2)) {
			yaml << *static_cast<glm::vec2*>(ptr);
		} else if (type == typeid(glm::vec3)) {
			yaml << *static_cast<glm::vec3*>(ptr);
		} else if (type == typeid(glm::vec4)) {
			yaml << *static_cast<glm::vec4*>(ptr);
		} else if (type == typeid(glm::mat4)) {
			yaml << *static_cast<glm::mat4*>(ptr);
		} else if (type == typeid(Guid)) {
			auto* guid = static_cast<Guid*>(ptr);
			if (guid->is_valid()) {
				yaml << guid->value;
			} else {
				yaml << YAML::Null;
			}
		} else {
			// Unknown type - skip
			yaml << YAML::Null;
		}
	}

	static bool deserialize_value(YAML::Node const& node, void* ptr, std::type_info const& type,
		FieldInfo const& field, const EnumInfo* enum_info) {

		if (field.is_guid) {
			auto* guid = static_cast<Guid*>(ptr);
			if (node.IsNull()) {
				guid->value = "";
			} else {
				guid->value = node.as<std::string>();
			}
			return true;
		}

		if (enum_info) {
			if (node.IsScalar()) {
				std::string name = node.as<std::string>();
				// Try name lookup first
				for (auto const& item : enum_info->items) {
					if (item.name == name) {
						*static_cast<int*>(ptr) = item.value;
						return true;
					}
				}
				// Fallback: parse as int
				try {
					*static_cast<int*>(ptr) = std::stoi(name);
					return true;
				} catch (...) {}
			}
			return false;
		}

		if (type == typeid(bool)) {
			*static_cast<bool*>(ptr) = node.as<bool>();
		} else if (type == typeid(int)) {
			*static_cast<int*>(ptr) = node.as<int>();
		} else if (type == typeid(uint32_t)) {
			*static_cast<uint32_t*>(ptr) = node.as<uint32_t>();
		} else if (type == typeid(uint8_t)) {
			*static_cast<uint8_t*>(ptr) = (uint8_t)node.as<int>();
		} else if (type == typeid(int8_t)) {
			*static_cast<int8_t*>(ptr) = (int8_t)node.as<int>();
		} else if (type == typeid(float)) {
			*static_cast<float*>(ptr) = node.as<float>();
		} else if (type == typeid(double)) {
			*static_cast<double*>(ptr) = node.as<double>();
		} else if (type == typeid(std::string)) {
			*static_cast<std::string*>(ptr) = node.as<std::string>();
		} else if (type == typeid(glm::vec2)) {
			*static_cast<glm::vec2*>(ptr) = node.as<glm::vec2>();
		} else if (type == typeid(glm::vec3)) {
			*static_cast<glm::vec3*>(ptr) = node.as<glm::vec3>();
		} else if (type == typeid(glm::vec4)) {
			*static_cast<glm::vec4*>(ptr) = node.as<glm::vec4>();
		} else if (type == typeid(Guid)) {
			*static_cast<Guid*>(ptr) = node.as<Guid>();
		} else {
			return false;
		}
		return true;
	}

	void serialize_field(YAML::Emitter& yaml, void* instance, FieldInfo const& field) {
		void* ptr = (uint8_t*)instance + field.offset;
		std::string yaml_key_str = field.yaml_key ? field.yaml_key : field_to_yaml_key(field.name);
		auto const yaml_key = yaml_key_str.c_str();

		if (field.container) {
			yaml << YAML::Key << yaml_key << YAML::Value;
			if (field.container->is_map) {
				// Map container: emit YAML map
				yaml << YAML::BeginMap;
				size_t size = field.container->size(ptr);
				for (size_t i = 0; i < size; ++i) {
					std::string key;
					if (field.container->get_key) {
						field.container->get_key(ptr, i, key);
					}
					void* elem_ptr = field.container->get(ptr, i);
					yaml << YAML::Key << key << YAML::Value;
					if (field.container->is_element_asset_ref) {
						std::string guid = asset_ref_to_guid_string(elem_ptr);
						if (!guid.empty()) {
							yaml << guid;
						} else {
							yaml << YAML::Null;
						}
					} else {
						serialize_value(yaml, elem_ptr, *field.container->element_type, field,
							field.container->element_enum_info);
					}
				}
				yaml << YAML::EndMap;
			}
			else {
				size_t size = field.container->size(ptr);
				if (field.container->is_array) {
					yaml << YAML::Flow;
				}
				yaml << YAML::BeginSeq;
				for (size_t i = 0; i < size; ++i) {
					void* elem_ptr = field.container->get(ptr, i);
					if (field.container->is_element_asset_ref) {
						std::string guid = asset_ref_to_guid_string(elem_ptr);
						if (!guid.empty()) {
							yaml << guid;
						} else {
							yaml << YAML::Null;
						}
					} else {
						serialize_value(yaml, elem_ptr, *field.container->element_type, field,
							field.container->element_enum_info);
					}
				}
				yaml << YAML::EndSeq;
			}
		} else if (field.custom_getter) {
			yaml << YAML::Key << yaml_key << YAML::Value;
			void* custom_ptr = field.custom_getter(instance);
			if (*field.type == typeid(std::vector<std::string>)) {
				auto* vec = static_cast<std::vector<std::string>*>(custom_ptr);
				yaml << YAML::BeginSeq;
				for (auto const& s : *vec) {
					yaml << s;
				}
				yaml << YAML::EndSeq;
			} else {
				serialize_value(yaml, custom_ptr, *field.type, field, field.enum_info);
			}
		} else {
			if (field.is_asset_ref) {
				yaml << YAML::Key << yaml_key << YAML::Value;
				std::string guid = asset_ref_to_guid_string(ptr);
				if (!guid.empty()) {
					yaml << guid;
				} else {
					yaml << YAML::Null;
				}
			} else {
				yaml << YAML::Key << yaml_key << YAML::Value;
				serialize_value(yaml, ptr, *field.type, field, field.enum_info);
			}
		}
	}

	bool deserialize_field(YAML::Node const& node, void* instance, FieldInfo const& field) {
		std::string yaml_key = field.yaml_key ? field.yaml_key : field_to_yaml_key(field.name);
		if (!node[yaml_key]) {
			return false; // Field not present in YAML, skip
		}

		auto const& value_node = node[yaml_key];

		void* ptr = (uint8_t*)instance + field.offset;

		if (field.container) {

			if (field.container->is_map) {
				// Map container: deserialize from YAML map
				if (value_node.IsMap()) {
					for (auto it = value_node.begin(); it != value_node.end(); ++it) {
						std::string key = it->first.as<std::string>();
						if (field.container->insert_or_assign) {
							field.container->insert_or_assign(ptr, key);
						}
						void* elem_ptr = nullptr;
						if (field.container->get_by_key) {
							elem_ptr = field.container->get_by_key(ptr, key);
						}
						if (elem_ptr) {
							if (field.container->is_element_asset_ref && field.container->load_asset_by_guid) {
								std::string guid_str = it->second.IsNull() ? "" : it->second.as<std::string>();
								field.container->load_asset_by_guid(elem_ptr, guid_str);
							} else if (field.container->is_element_asset_ref) {
								std::string guid_str = it->second.IsNull() ? "" : it->second.as<std::string>();
								asset_ref_from_guid_string(elem_ptr, guid_str, field.get_widget_value<std::string>("type", ""));
							} else {
								deserialize_value(it->second, elem_ptr, *field.container->element_type, field,
									field.container->element_enum_info);
							}
						}
					}
				}
				return true;
			}

			size_t expected_size = 0;
			if (value_node.IsSequence()) {
				expected_size = value_node.size();
			}
			if (!field.container->is_array && expected_size > 0) {
				field.container->resize(ptr, expected_size);
			}
			size_t actual_size = field.container->size(ptr);
			for (size_t i = 0; i < actual_size && i < expected_size; ++i) {
				void* elem_ptr = field.container->get(ptr, i);
				if (field.container->is_element_asset_ref && field.container->load_asset_by_guid) {
					std::string guid_str = value_node[i].IsNull() ? "" : value_node[i].as<std::string>();
					field.container->load_asset_by_guid(elem_ptr, guid_str);
				} else if (field.container->is_element_asset_ref) {
					std::string guid_str = value_node[i].IsNull() ? "" : value_node[i].as<std::string>();
					asset_ref_from_guid_string(elem_ptr, guid_str, field.get_widget_value<std::string>("type", ""));
				} else {
					deserialize_value(value_node[i], elem_ptr, *field.container->element_type, field,
						field.container->element_enum_info);
				}
			}
			return true;
		}

		if (field.is_asset_ref) {
			// Asset ref deserialization needs type-specific logic (lookup by guid)
			// Handled in the scene load path for now
			return false;
		}

		if (field.custom_getter) {
			// Custom setter path not used during deserialization - handled specially
			return false;
		}

		return deserialize_value(value_node, ptr, *field.type, field, field.enum_info);
	}

	void serialize_type(YAML::Emitter& yaml, void* instance, std::string const& type_name) {
		auto const* info = TypeRegistry::instance().get(type_name);
		if (!info) return;

		yaml << YAML::BeginMap;
		for (auto const& field : info->fields) {
			if ((field.flag & FF_Serializable) == 0) continue;
			serialize_field(yaml, instance, field);
		}
		yaml << YAML::EndMap;
	}

	void deserialize_type(YAML::Node const& node, void* instance, std::string const& type_name) {
		auto const* info = TypeRegistry::instance().get(type_name);
		if (!info) return;

		for (auto const& field : info->fields) {
			if ((field.flag & FF_Serializable) == 0) continue;
			deserialize_field(node, instance, field);
		}
	}

	std::string type_to_yaml_key(std::string const& type_name) {
		std::string name = type_name;
		// Strip trailing "Component" suffix
		if (name.size() > 9 && name.substr(name.size() - 9) == "Component") {
			name = name.substr(0, name.size() - 9);
		}
		return field_to_yaml_key(name);
	}

	std::string asset_ref_to_guid_string(void* shared_ptr_to_asset) {
		// Extract GUID from a shared_ptr<AssetT> by accessing the underlying AssetMeta.
		// shared_ptr layout: first pointer-sized field is the raw object pointer.
		// AssetBase has a virtual dtor, so the vtable pointer is at object offset 0;
		// AssetMeta m_meta is at offset sizeof(void*).
		void* const* raw = static_cast<void* const*>(shared_ptr_to_asset);
		void* object = raw ? *raw : nullptr;
		if (!object) return "";

		// Skip vtable pointer to reach AssetBase::m_meta
		uint8_t* meta_addr = static_cast<uint8_t*>(object) + sizeof(void*);
		auto* meta = reinterpret_cast<AssetMeta*>(meta_addr);
		return meta->guid.value;
	}

	bool asset_ref_from_guid_string(void* shared_ptr_to_asset, std::string const& guid_str, std::string const& asset_meta_type) {
		if (guid_str.empty()) return false;

		Guid guid = Guid::make(guid_str);
		if (!guid.is_valid()) return false;

		// shared_ptr layout: { T* ptr, control_block* ctrl }
		void** dst = static_cast<void**>(shared_ptr_to_asset);
		if (!dst) return false;

		if (asset_meta_type == "static mesh") {
			auto asset = StaticMesh::load(guid);
			if (asset) new (dst) std::shared_ptr<StaticMesh>(std::move(asset));
			return asset != nullptr;
		} else if (asset_meta_type == "skeletal mesh") {
			auto asset = SkeletalMesh::load(guid);
			if (asset) new (dst) std::shared_ptr<SkeletalMesh>(std::move(asset));
			return asset != nullptr;
		} else if (asset_meta_type == "texture2d") {
			auto asset = Texture2D::load(guid);
			if (asset) new (dst) std::shared_ptr<Texture2D>(std::move(asset));
			return asset != nullptr;
		} else if (asset_meta_type == "animation") {
			auto asset = Animation::load(guid);
			if (asset) new (dst) std::shared_ptr<Animation>(std::move(asset));
			return asset != nullptr;
		} else if (asset_meta_type == "skeleton") {
			auto asset = Skeleton::load(guid);
			if (asset) new (dst) std::shared_ptr<Skeleton>(std::move(asset));
			return asset != nullptr;
		} else if (asset_meta_type == "material") {
			auto asset = Material::load(guid);
			if (asset) new (dst) std::shared_ptr<Material>(std::move(asset));
			return asset != nullptr;
		} else if (asset_meta_type == "material instance") {
			auto asset = MaterialInstance::load(guid);
			if (asset) new (dst) std::shared_ptr<MaterialInstance>(std::move(asset));
			return asset != nullptr;
		}
		return false;
	}

}
