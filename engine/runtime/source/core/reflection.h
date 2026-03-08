#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace z1 {

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

		template<typename T, typename C>
		T& get(C* instance) const {
			return *reinterpret_cast<T*>((uint8_t*)instance + offset);
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

	struct TypeInfo
	{
		std::string name;
		std::unordered_set<std::string> field_names;
		std::vector<FieldInfo> fields;
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

	private:
		std::unordered_map<std::string, TypeInfo> m_types;
	};

}
