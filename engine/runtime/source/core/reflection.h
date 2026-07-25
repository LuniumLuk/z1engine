#pragma once

#include <string>
#include <vector>
#include <array>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <memory>
#include "core/guid.h"

namespace z1 {

	// Forward declarations for asset-type trait
	struct Texture2D;
	struct StaticMesh;
	struct SkeletalMesh;
	struct Animation;
	struct Material;
	struct MaterialInstance;
	struct Shader;
	struct Skeleton;
	struct Guid;

	// Asset-type trait: marks types that are engine assets (stored as shared_ptr in components)
	template<typename T>
	struct is_asset_type : std::false_type {};

	template<> struct is_asset_type<Texture2D> : std::true_type {};
	template<> struct is_asset_type<StaticMesh> : std::true_type {};
	template<> struct is_asset_type<SkeletalMesh> : std::true_type {};
	template<> struct is_asset_type<Animation> : std::true_type {};
	template<> struct is_asset_type<Material> : std::true_type {};
	template<> struct is_asset_type<MaterialInstance> : std::true_type {};
	template<> struct is_asset_type<Shader> : std::true_type {};
	template<> struct is_asset_type<Skeleton> : std::true_type {};

	template<typename T>
	inline constexpr bool is_asset_type_v = is_asset_type<T>::value;

	// Helper to detect asset-ref fields (shared_ptr<AssetType>)
	template<typename T>
	struct is_asset_ref_field : std::false_type {};

	template<typename T>
	struct is_asset_ref_field<std::shared_ptr<T>> : is_asset_type<T> {};

	template<typename T>
	inline constexpr bool is_asset_ref_field_v = is_asset_ref_field<T>::value;

	struct Entity;

	enum FieldFlags : uint32_t
	{
		FF_None             = 0,
		FF_Visible          = 1 << 0,
		FF_Editable         = 1 << 1,
		FF_Serializable     = 1 << 2,
		FF_Copiable         = 1 << 3,
		FF_ContainerResizable = 1 << 4,  // container can be resized from editor (e.g., add/remove elements)
		// combinations
		FF_Hidden           = FF_None,
		FF_ReadOnly         = FF_Visible | FF_Copiable,
		FF_Default          = FF_Visible | FF_Editable | FF_Serializable | FF_Copiable,
	};

	// YAML key override: when set, use this key instead of the auto-derived snake_case name
	inline constexpr const char* YAML_KEY_OVERRIDE_NONE = nullptr;

	struct EnumItem {
		std::string name;
		int value;
	};

	struct EnumInfo {
		std::string name;
		std::vector<EnumItem> items;
	};

	struct ContainerInfo
	{
		bool is_array;
		bool is_map = false;               // true for unordered_map
		bool is_element_asset_ref = false; // true if element type is shared_ptr<AssetType>
		size_t (*size)(const void* instance);
		void* (*get)(void* instance, size_t index);
		void (*resize)(void* instance, size_t new_size);
		const std::type_info* element_type;
		const std::type_info* key_type = nullptr;  // for maps: typeid of key
		const EnumInfo* element_enum_info;

		// Map-specific: get key string at iteration index
		void (*get_key)(const void* instance, size_t index, std::string& out_key) = nullptr;
		// Map-specific: get value by key (returns nullptr if not found)
		void* (*get_by_key)(void* instance, const std::string& key) = nullptr;
		// Map-specific: insert or assign key with default-constructed value
		void (*insert_or_assign)(void* instance, const std::string& key) = nullptr;
		// Map-specific: erase entry by key
		void (*erase_key)(void* instance, const std::string& key) = nullptr;

		// Load asset by guid string and assign to element via typed operator=.
		// Set by ContainerInfoResolver when is_element_asset_ref is true.
		// Uses the concrete shared_ptr<T> type to call T::load(guid) and properly
		// assign, destroying any previous value.
		bool (*load_asset_by_guid)(void* elem_ptr, std::string const& guid_str) = nullptr;
	};

	struct FieldInfo
	{
		std::string name;
		size_t offset;
		size_t size;
		const std::type_info* type;
		uint32_t flag;
		// widget string for editor customization
		// e.g. "type=slider;min=0;max=100;step=1"
		std::string widget;
		const ContainerInfo* container = nullptr;
		const EnumInfo* enum_info = nullptr;

		// Custom accessor callbacks for fields that cannot be modeled as offset+typeid
		// (e.g., Material::Variable::Value tagged union, ScriptComponent script entries)
		using GetterFn = std::function<void*(void* instance)>;
		using SetterFn = std::function<void(void* instance, void const* value)>;
		using ClearFn = std::function<void(void* instance)>;
		GetterFn custom_getter = nullptr;
		SetterFn custom_setter = nullptr;

		// For asset-ref fields: properly resets the shared_ptr (calls .reset())
		// Avoids directly zeroing shared_ptr internals which crashes
		ClearFn clear_fn = nullptr;

		// YAML key override: if non-null, use this string as the YAML map key
		// instead of auto-deriving from field name (strip m_, snake_case)
		const char* yaml_key = nullptr;

		// Whether this field is an asset reference (shared_ptr<T> where is_asset_type_v<T>)
		bool is_asset_ref = false;

		// Whether this field is of type Guid (for read-only display in editor, string in YAML)
		bool is_guid = false;

		template<typename T, typename C>
		T& get(C* instance) const {
			if (custom_getter) {
				return *reinterpret_cast<T*>(custom_getter(instance));
			}
			return *reinterpret_cast<T*>((uint8_t*)instance + offset);
		}

		template<typename T, typename C>
		T const& get_const(C const* instance) const {
			if (custom_getter) {
				return *reinterpret_cast<T const*>(custom_getter(const_cast<void*>((void const*)instance)));
			}
			return *reinterpret_cast<T const*>((uint8_t const*)instance + offset);
		}

		bool is_widget_type(std::string const& type_name) const {
			return widget.find("[" + type_name + "]") != std::string::npos;
		}

		template<typename T>
		T get_widget_value(std::string const& key, T default_value) const {
			size_t pos = widget.find(key + "=");
			if (pos != std::string::npos) {
				size_t end = widget.find(";", pos);
				std::string value_str = widget.substr(pos + key.length() + 1, end - (pos + key.length() + 1));
				if constexpr (std::is_same<T, int>::value) {
					return std::stoi(value_str);
				}
				else if constexpr (std::is_same<T, float>::value) {
					return std::stof(value_str);
				}
				else if constexpr (std::is_same<T, bool>::value) {
					return value_str == "true" || value_str == "1";
				}
				else if constexpr (std::is_same<T, std::string>::value) {
					return value_str;
				}
			}
			return default_value;
		}
	};

	// Forward declarations for field metadata helpers
	template<typename T> struct ContainerInfoResolver;
	template<typename T> struct EnumInfoResolver;

	// Helper to configure field metadata at registration time
	template<typename FieldType>
	inline void configure_field_meta(FieldInfo& field_info) {
		field_info.container = ContainerInfoResolver<FieldType>::get();
		field_info.enum_info = EnumInfoResolver<FieldType>::get();
		field_info.is_guid = std::is_same_v<FieldType, Guid>;
		field_info.is_asset_ref = is_asset_ref_field_v<FieldType>;
		// For asset-ref fields, register a clear callback that properly resets the shared_ptr
		if constexpr (is_asset_ref_field_v<FieldType>) {
			field_info.clear_fn = [](void* instance) {
				auto* sp = reinterpret_cast<FieldType*>(instance);
				sp->reset();
			};
		}
	}

	struct TypeInfo
	{
		std::string name;
		std::unordered_set<std::string> field_names;
		std::vector<FieldInfo> fields;

		// Type-erased hooks
		std::function<void(void* buffer)> construct = nullptr;
		std::function<void(Entity& entity)> add_to = nullptr;
		std::function<void(Entity& entity)> remove_from = nullptr;
		std::function<bool(Entity const& entity)> has_in = nullptr;

		bool is_component() const { return add_to != nullptr; }
	};

	struct TypeRegistry
	{
		TypeRegistry();

		static TypeRegistry& instance() {
			static TypeRegistry r;
			return r;
		}

		void register_type(std::string const& name);
		void register_field(std::string const& name, FieldInfo const& field);

		const TypeInfo* get(std::string const& name) const;

		// Iterate all registered types (for editor menus, etc.)
		std::vector<std::string> get_all_type_names() const;

		// Get all component types (types with add_to hook)
		std::vector<const TypeInfo*> get_all_components() const;

	private:
		std::unordered_map<std::string, TypeInfo> m_types;
	};

	template<typename T>
	struct ContainerInfoResolver {
		static const ContainerInfo* get() {
			return nullptr;
		}
	};

	template<typename T, typename Alloc>
	struct ContainerInfoResolver<std::vector<T, Alloc>> {
		static const ContainerInfo* get() {
			static const ContainerInfo info = []() {
				ContainerInfo info{
					false,  // is_array
					false,  // is_map
					is_asset_ref_field_v<T>,  // is_element_asset_ref
					[](const void* instance) { return reinterpret_cast<const std::vector<T, Alloc>*>(instance)->size(); },
					[](void* instance, size_t index) { return reinterpret_cast<void*>(&(*reinterpret_cast<std::vector<T, Alloc>*>(instance))[index]); },
					[](void* instance, size_t new_size) { reinterpret_cast<std::vector<T, Alloc>*>(instance)->resize(new_size); },
					&typeid(T),
					nullptr, // key_type
					EnumInfoResolver<T>::get()
				};
				if constexpr (is_asset_ref_field_v<T>) {
					info.load_asset_by_guid = [](void* elem_ptr, std::string const& guid_str) -> bool {
						using AssetType = typename T::element_type;
						if (guid_str.empty()) return false;
						Guid guid = Guid::make(guid_str);
						if (!guid.is_valid()) return false;
						auto asset = AssetType::load(guid);
						if (asset) {
							*static_cast<T*>(elem_ptr) = std::move(asset);
							return true;
						}
						return false;
					};
				}
				return info;
			}();
			return &info;
		}
	};

	template<typename T, size_t N>
	struct ContainerInfoResolver<std::array<T, N>> {
		static const ContainerInfo* get() {
			static const ContainerInfo info = []() {
				ContainerInfo info{
					true,   // is_array
					false,  // is_map
					is_asset_ref_field_v<T>,  // is_element_asset_ref
					[](const void* instance) { return N; },
					[](void* instance, size_t index) { return reinterpret_cast<void*>(&(*reinterpret_cast<std::array<T, N>*>(instance))[index]); },
					nullptr, // resize
					&typeid(T),
					nullptr, // key_type
					EnumInfoResolver<T>::get()
				};
				if constexpr (is_asset_ref_field_v<T>) {
					info.load_asset_by_guid = [](void* elem_ptr, std::string const& guid_str) -> bool {
						using AssetType = typename T::element_type;
						if (guid_str.empty()) return false;
						Guid guid = Guid::make(guid_str);
						if (!guid.is_valid()) return false;
						auto asset = AssetType::load(guid);
						if (asset) {
							*static_cast<T*>(elem_ptr) = std::move(asset);
							return true;
						}
						return false;
					};
				}
				return info;
			}();
			return &info;
		}
	};

	template<typename K, typename V, typename Hash, typename KeyEqual, typename Alloc>
	struct ContainerInfoResolver<std::unordered_map<K, V, Hash, KeyEqual, Alloc>> {
		static const ContainerInfo* get() {
			static const ContainerInfo info = []() {
				ContainerInfo info{
					false,  // is_array
					true,   // is_map
					is_asset_ref_field_v<V>,  // is_element_asset_ref
					// size
					[](const void* instance) {
						return reinterpret_cast<const std::unordered_map<K, V, Hash, KeyEqual, Alloc>*>(instance)->size();
					},
					// get by index (iterate to index)
					[](void* instance, size_t index) -> void* {
						auto* map = reinterpret_cast<std::unordered_map<K, V, Hash, KeyEqual, Alloc>*>(instance);
						size_t i = 0;
						for (auto& pair : *map) {
							if (i == index) return &pair.second;
							++i;
						}
						return nullptr;
					},
					nullptr, // resize (not supported for maps)
					&typeid(V),
					&typeid(K), // key_type
					EnumInfoResolver<V>::get(),
					// get_key
					[](const void* instance, size_t index, std::string& out_key) {
						auto* map = reinterpret_cast<const std::unordered_map<K, V, Hash, KeyEqual, Alloc>*>(instance);
						size_t i = 0;
						for (auto const& pair : *map) {
							if (i == index) {
								if constexpr (std::is_same_v<K, std::string>) {
									out_key = pair.first;
								}
								else {
									out_key = std::to_string(pair.first);
								}
								return;
							}
							++i;
						}
					},
					// get_by_key
					[](void* instance, const std::string& key) -> void* {
						auto* map = reinterpret_cast<std::unordered_map<K, V, Hash, KeyEqual, Alloc>*>(instance);
						if constexpr (std::is_same_v<K, std::string>) {
							auto it = map->find(key);
							if (it != map->end()) return &it->second;
						}
						return nullptr;
					},
					// insert_or_assign
					[](void* instance, const std::string& key) {
						auto* map = reinterpret_cast<std::unordered_map<K, V, Hash, KeyEqual, Alloc>*>(instance);
						if constexpr (std::is_same_v<K, std::string>) {
							(*map)[key] = V{};
						}
					},
					// erase_key
					[](void* instance, const std::string& key) {
						auto* map = reinterpret_cast<std::unordered_map<K, V, Hash, KeyEqual, Alloc>*>(instance);
						if constexpr (std::is_same_v<K, std::string>) {
							map->erase(key);
						}
					}
				};
				if constexpr (is_asset_ref_field_v<V>) {
					info.load_asset_by_guid = [](void* elem_ptr, std::string const& guid_str) -> bool {
						using AssetType = typename V::element_type;
						if (guid_str.empty()) return false;
						Guid guid = Guid::make(guid_str);
						if (!guid.is_valid()) return false;
						auto asset = AssetType::load(guid);
						if (asset) {
							*static_cast<V*>(elem_ptr) = std::move(asset);
							return true;
						}
						return false;
					};
				}
				return info;
			}();
			return &info;
		}
	};

	struct EnumRegistry {
		static EnumRegistry& instance() {
			static EnumRegistry r;
			return r;
		}

		template<typename T>
		void register_item(std::string const& type_name, std::string const& name, T value) {
			size_t hash = typeid(T).hash_code();
			if (m_enums.find(hash) == m_enums.end()) {
				m_enums[hash].name = type_name;
			}
			auto& items = m_enums[hash].items;
			for (auto const& item : items) {
				if (item.name == name) {
					return;
				}
			}
			items.push_back({ name, (int)value });
		}

		template<typename T>
		const EnumInfo* get() const {
			auto it = m_enums.find(typeid(T).hash_code());
			if (it != m_enums.end()) {
				return &it->second;
			}
			return nullptr;
		}

	private:
		std::unordered_map<size_t, EnumInfo> m_enums;
	};

	template<typename T>
	struct EnumInfoResolver {
		static const EnumInfo* get() {
			return EnumRegistry::instance().get<T>();
		}
	};

}
