#include "z1engine.h"

using namespace z1;

struct OurApp : Application {
	void init() override {};
};

int main() {
	OurApp app;
	app.init();

	int failures = 0;

	// remove leftovers from a previous crashed run
	std::filesystem::remove("content/test_material_roundtrip.yaml");

	auto shader_guid = g_runtime_context.m_asset_manager->resolve_guid("shader/pbr");
	if (!shader_guid.is_valid()) {
		std::cerr << "FAIL: shader/pbr not resolved" << std::endl;
		return 1;
	}

	auto mat = Material::create(Filepath("test_material_roundtrip"), MaterialFlags::Scene, shader_guid);
	if (!mat) {
		std::cerr << "FAIL: failed to create material" << std::endl;
		return 1;
	}

	// pick the first float variable and edit it
	std::string var_name;
	for (auto const& [name, var] : mat->m_variables) {
		if (var.visible && var.type == DataType::Float) {
			var_name = name;
			break;
		}
	}
	if (var_name.empty()) {
		std::cerr << "FAIL: no float variable found" << std::endl;
		++failures;
	}
	else {
		auto& var = mat->m_variables[var_name];
		var.default_value.vec[0] = 0.123f;
		var.default_value.valid = true;
	}

	mat->save();

	auto reloaded = Asset<Material>::load(mat->m_meta.guid);
	if (!reloaded) {
		std::cerr << "FAIL: failed to reload material" << std::endl;
		++failures;
	}
	else if (!var_name.empty()) {
		auto& rv = reloaded->m_variables[var_name].default_value;
		if (!rv.valid || std::abs(rv.vec[0] - 0.123f) > 1e-6f) {
			std::cerr << "FAIL: variable value did not round-trip: " << rv.vec[0] << std::endl;
			++failures;
		}
	}

	// cleanup
	auto file = g_runtime_context.m_asset_manager->get_file_from_guid(mat->m_meta.guid);
	if (!file.empty()) {
		std::filesystem::remove(file.string() + ".yaml");
	}

	if (failures == 0) {
		std::cout << "test_material_roundtrip: all checks passed" << std::endl;
	}
	return failures;
}
