#include "pch.h"
#include "core/reflection.h"
#include "core/log.h"

namespace z1 {

	TypeRegistry::TypeRegistry() {
		g_runtime_context.init_logger();
	}

	void TypeRegistry::register_type(std::string const& name) {
		if (m_types.find(name) == m_types.end()) {
			m_types[name] = { name, {} };
			CORE_INFO("registered type '{}'", name);
		}
	}

	void TypeRegistry::register_field(std::string const& name, const FieldInfo& field) {
		if (m_types.find(name) == m_types.end()) {
			CORE_ERROR("type '{}' not registered before registering field '{}'", name, field.name);
			return;
		}

		auto& type_info = m_types[name];
		if (type_info.field_names.find(field.name) == type_info.field_names.end()) {
			type_info.field_names.insert(field.name);
			type_info.fields.push_back(field);
			std::sort(type_info.fields.begin(), type_info.fields.end(),
				[](FieldInfo const& lhs, FieldInfo const& rhs) {
					return lhs.offset < rhs.offset;
				});
			CORE_INFO("registered field '{}' in type '{}'", field.name, name);
		}
	}

	const TypeInfo* TypeRegistry::get(const std::string& name) const {
		auto it = m_types.find(name);
		return it != m_types.end() ? &it->second : nullptr;
	}

}
