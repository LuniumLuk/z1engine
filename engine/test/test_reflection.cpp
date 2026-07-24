#include "z1engine.h"
#include "core/reflection.h"
#include "render/data_types.h"
#include "scene/component/light.h"
#include "render/global.h"

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

using namespace z1;

int main() {
	auto& registry = TypeRegistry::instance();

	// 1. Verify all expected component types are registered
	std::vector<std::string> expected_components = {
		"TagComponent",
		"TransformComponent",
		"CameraComponent",
		"LightComponent",
		"SpriteComponent",
		"SkyLightComponent",
		"AnimationComponent",
		"ParticleComponent",
		"PostprocessVolumeComponent",
		"StaticMeshComponent",
		"SkeletalMeshComponent",
		"ScriptComponent",
	};

	auto components = registry.get_all_components();
	std::vector<std::string> registered_names;
	for (auto* info : components) {
		registered_names.push_back(info->name);
	}

	int failures = 0;
	for (auto const& expected : expected_components) {
		bool found = false;
		for (auto* info : components) {
			if (info->name == expected) {
				found = true;
				break;
			}
		}
		if (!found) {
			std::cerr << "FAIL: component '" << expected << "' not found in TypeRegistry" << std::endl;
			++failures;
		}
	}

	// 2. Verify GlobalSettings is registered
	auto* gs_info = registry.get("GlobalSettings");
	if (!gs_info) {
		std::cerr << "FAIL: GlobalSettings not found in TypeRegistry" << std::endl;
		++failures;
	}
	else {
		// Verify key fields exist
		std::vector<std::string> expected_fields = {
			"sun_direction", "sun_color", "sun_intensity",
			"taa_enabled", "taa_blend",
			"pp_exposure", "pp_gamma", "pp_tint",
			"sm_near", "sm_far", "sm_ortho_size",
			"script_enabled", "render_mode",
		};
		for (auto const& field_name : expected_fields) {
			bool found = false;
			for (auto const& f : gs_info->fields) {
				if (f.name == field_name) {
					found = true;
					break;
				}
			}
			if (!found) {
				std::cerr << "FAIL: GlobalSettings field '" << field_name << "' not found" << std::endl;
				++failures;
			}
		}
	}

	// 3. Verify component hooks are set (add_to/remove_from/has_in)
	for (auto* info : components) {
		if (info->name == "ScriptComponent") {
			// ScriptComponent has manually-registered hooks, verify they exist
			if (!info->add_to || !info->remove_from || !info->has_in) {
				std::cerr << "FAIL: ScriptComponent missing hooks" << std::endl;
				++failures;
			}
		}
		else if (info->name == "TagComponent" || info->name == "TransformComponent") {
			// These always exist, verify hooks
			if (!info->add_to || !info->remove_from || !info->has_in) {
				std::cerr << "FAIL: " << info->name << " missing hooks" << std::endl;
				++failures;
			}
		}
	}

	// 4. Verify Material/MaterialInstance are registered
	auto* mat_info = registry.get("Material");
	if (!mat_info) {
		std::cerr << "FAIL: Material not found in TypeRegistry" << std::endl;
		++failures;
	}
	else {
		bool has_flags = false, has_shader = false;
		for (auto const& f : mat_info->fields) {
			if (f.name == "m_flags") has_flags = true;
			if (f.name == "m_shader_guid") has_shader = true;
		}
		if (!has_flags) { std::cerr << "FAIL: Material missing m_flags field" << std::endl; ++failures; }
		if (!has_shader) { std::cerr << "FAIL: Material missing m_shader_guid field" << std::endl; ++failures; }
	}

	auto* mi_info = registry.get("MaterialInstance");
	if (!mi_info) {
		std::cerr << "FAIL: MaterialInstance not found in TypeRegistry" << std::endl;
		++failures;
	}

	// 5. Verify EditorCameraData is registered
	auto* ec_info = registry.get("EditorCameraData");
	if (!ec_info) {
		std::cerr << "FAIL: EditorCameraData not found in TypeRegistry" << std::endl;
		++failures;
	}

	// 6. Verify enum registrations
	auto const& enum_registry = EnumRegistry::instance();
	auto* dt_enum = enum_registry.get<DataType>();
	if (!dt_enum) {
		std::cerr << "FAIL: DataType enum not registered" << std::endl;
		++failures;
	}

	auto* lt_enum = enum_registry.get<LightType>();
	if (!lt_enum) {
		std::cerr << "FAIL: LightType enum not registered" << std::endl;
		++failures;
	}

	auto* rm_enum = enum_registry.get<RenderMode>();
	if (!rm_enum) {
		std::cerr << "FAIL: RenderMode enum not registered" << std::endl;
		++failures;
	}

	if (failures == 0) {
		std::cout << "OK: All " << expected_components.size() << " components, GlobalSettings, EditorCameraData, "
			<< "Material, MaterialInstance, and 3 enums verified in TypeRegistry ("
			<< registered_names.size() << " total component types)" << std::endl;
		return 0;
	}

	std::cerr << "FAILED: " << failures << " registration check(s) failed" << std::endl;
	return 1;
}
