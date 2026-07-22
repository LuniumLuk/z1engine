#pragma once

#include <string>
#include <vector>
#include <array>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <memory>

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
		size_t (*size)(const void* instance);
		void* (*get)(void* instance, size_t index);
		void (*resize)(void* instance, size_t new_size);
		const std::type_info* element_type;
		const EnumInfo* element_enum_info;
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
			static ContainerInfo info{
				false,
				[](const void* instance) { return reinterpret_cast<const std::vector<T, Alloc>*>(instance)->size(); },
				[](void* instance, size_t index) { return reinterpret_cast<void*>(&(*reinterpret_cast<std::vector<T, Alloc>*>(instance))[index]); },
				[](void* instance, size_t new_size) { reinterpret_cast<std::vector<T, Alloc>*>(instance)->resize(new_size); },
				&typeid(T),
				EnumInfoResolver<T>::get()
			};
			return &info;
		}
	};

	template<typename T, size_t N>
	struct ContainerInfoResolver<std::array<T, N>> {
		static const ContainerInfo* get() {
			static ContainerInfo info{
				true,
				[](const void* instance) { return N; },
				[](void* instance, size_t index) { return reinterpret_cast<void*>(&(*reinterpret_cast<std::array<T, N>*>(instance))[index]); },
				nullptr,
				&typeid(T),
				EnumInfoResolver<T>::get()
			};
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
