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

		template<typename T, typename C>
		T& get(C* instance) const {
			return *reinterpret_cast<T*>((uint8_t*)instance + offset);
		}
	};

	struct TypeInfo
	{
		std::string name;
		std::unordered_map<std::string, FieldInfo> fields;
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
