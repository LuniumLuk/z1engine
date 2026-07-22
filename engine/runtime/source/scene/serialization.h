#pragma once

#include "core/reflection.h"
#include "util/yaml.h"
#include <yaml-cpp/yaml.h>
#include <string>

namespace z1 {

	// Derive YAML key from field name:
	// - Strip leading "m_"
	// - Convert CamelCase to snake_case
	std::string field_to_yaml_key(std::string const& field_name);

	// Serialize a single field's value to YAML emitter
	// Handles: bool, int, float, vec2/3/4, string, Guid, asset refs, enums, containers
	void serialize_field(YAML::Emitter& yaml, void* instance, FieldInfo const& field);

	// Deserialize a single field's value from a YAML node
	// Returns true on success
	bool deserialize_field(YAML::Node const& node, void* instance, FieldInfo const& field);

	// Serialize all serializable fields of a type to YAML
	void serialize_type(YAML::Emitter& yaml, void* instance, std::string const& type_name);

	// Deserialize all serializable fields of a type from YAML
	void deserialize_type(YAML::Node const& node, void* instance, std::string const& type_name);

	// Derive YAML key from type name:
	// - Strip trailing "Component" suffix
	// - Convert CamelCase to snake_case
	std::string type_to_yaml_key(std::string const& type_name);

	// Serialize an asset reference GUID: resolves shared_ptr<AssetT> to its GUID string
	// Returns the GUID string or empty if null
	std::string asset_ref_to_guid_string(void* shared_ptr_to_asset);

	// Deserialize an asset reference: loads asset from GUID string
	// Stores result back into the shared_ptr field
	bool asset_ref_from_guid_string(void* shared_ptr_to_asset, std::string const& guid_str, std::string const& asset_meta_type);

}
