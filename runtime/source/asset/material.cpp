#include "pch.h"

#include "util/string_utils.h"
#include "asset/material.h"

namespace z1 {

	static DataType data_type_from_str(const std::string& type_str) {
		if (type_str == "int") return DataType::Int;
		if (type_str == "ivec2") return DataType::Int2;
		if (type_str == "ivec3") return DataType::Int3;
		if (type_str == "ivec4") return DataType::Int4;
		if (type_str == "float") return DataType::Float;
		if (type_str == "vec2") return DataType::Float2;
		if (type_str == "vec3") return DataType::Float3;
		if (type_str == "vec4") return DataType::Float4;
		if (type_str == "mat3") return DataType::Mat3;
		if (type_str == "mat4") return DataType::Mat4;
		return DataType::None; // unknown type
	}

	Material::Material(Pipeline::Description const& pipeline_desc)
		: m_pipeline_desc(pipeline_desc) {

		m_pipeline = Pipeline::build(pipeline_desc);
		auto const& shader = m_pipeline->m_shader;

		for (auto& uniform : shader->get_uniforms()) {
			if (uniform.m_location == INVALID_LOCATION) continue;
			if (uniform.m_type == DataType::None) continue;

			Variable v{};
			v.name = uniform.m_name;
			v.type = uniform.m_type;
			v.location = uniform.m_location;
			v.count = uniform.m_count;
			if (v.type == DataType::Mat3 || v.type == DataType::Mat4) {
				// matrices are not supposed to be set as material variables
				// they should be managed by engine internally
				v.visible = false;
			}
			if (v.type == DataType::Sampler2D || v.type == DataType::SamplerCube) {
				v.default_value.resource_id = INVALID_LOCATION;
			}
			m_variables[v.name] = v;
		}

		// parse reflections from shader file
		auto code = g_runtime_context.m_file_system->read_file(shader->get_path());
		size_t pos = 0;

		const char* reflection_token = "@reflections:";
		const size_t reflection_token_len = strlen(reflection_token);

		// find reflections
		std::string reflections;
		pos = code.find(reflection_token, pos);
		if (pos != std::string::npos) {
			size_t bracket_beg = code.find('{', pos);
			size_t bracket_end = find_paired_brackets(code, bracket_beg);
			reflections = code.substr(bracket_beg + 1, bracket_end - bracket_beg - 1);
		}

		reflections = remove_comments(reflections);

		for (auto const& line : split(reflections, '\n')) {
			if (is_blank(line)) continue;
			parse_reflection_line(line);
		}

		// we ignore uniform blocks, since most of the material variables
		// are just uniforms (OpenGL), or push constants (Vulkan)
		// in most case, the uniform blocks are used in engine internally
	}

	void Material::parse_reflection_line(const std::string& line) {
		std::istringstream iss(line);
		std::string token;

		std::string name;
		std::string type;
		std::vector<std::string> params;

		iss >> name;
		if (m_variables.find(name) == m_variables.end()) {
			CORE_WARN("reflected variable: {0} not found in shader variables!", name);
			return;
		}

		auto& var = m_variables[name];

		// parse flags (if present)
		iss >> std::ws;
		if (iss.peek() == '[') {
			iss.ignore(); // skip '['
			std::string flag_str;
			std::getline(iss, flag_str, ']');

			// split flags by any whitespace or comma
			std::istringstream flag_stream(flag_str);
			std::string flag;
			while (flag_stream >> flag) {
				// remove commas if present
				flag.erase(std::remove(flag.begin(), flag.end(), ','), flag.end());
				if (!flag.empty()) {
					if (flag == "invisible") {
						var.visible = false;
					}
				}
			}
		}

		// parse type and parameters (if present)
		iss >> std::ws;
		if (iss >> token && token == "=") {
			iss >> token; // read type
			type = token;
			if (var.type != data_type_from_str(type)) {
				CORE_WARN("reflected variable: {0} has inconsistent type {1} with shader variable {2}!", name, type, get_data_type_name(var.type));
				return;
			}

			// read parameters
			while (iss >> token) {
				if (!is_number(token)) {
					CORE_WARN("reflected variable: {0} has non-number param: {1}", name, token);
					return;
				}
				params.push_back(token);
			}
		}

		if (params.empty()) {
			return;
		}

		if (params.size() != get_data_type_element_count(var.type)) {
			CORE_WARN("reflected variable: {0} has inconsistent element count {1} with shader variable {2}!", name, params.size(), get_data_type_element_count(var.type));
			return;
		}

		var.default_value.valid = true;

		switch (var.type) {
		case DataType::Int4:
			var.default_value.ivec[3] = std::stoi(params[3]);
			/* fallthrough */
		case DataType::Int3:
			var.default_value.ivec[2] = std::stoi(params[2]);
			/* fallthrough */
		case DataType::Int2:
			var.default_value.ivec[1] = std::stoi(params[1]);
			/* fallthrough */
		case DataType::Int:
			var.default_value.ivec[0] = std::stoi(params[0]);
			break;
		case DataType::Float4:
			var.default_value.vec[3] = std::stof(params[3]);
			/* fallthrough */
		case DataType::Float3:
			var.default_value.vec[2] = std::stof(params[2]);
			/* fallthrough */
		case DataType::Float2:
			var.default_value.vec[1] = std::stof(params[1]);
			/* fallthrough */
		case DataType::Float:
			var.default_value.vec[0] = std::stof(params[0]);
			break;
		case DataType::Mat3:
		case DataType::Mat4:
			// mat3 and mat4 are not supposed to be set as material variables
			// they should be managed by engine internally
			var.visible = false;
			break;
		}
	}

	MaterialInstance::MaterialInstance(std::shared_ptr<Material> const& material)
		: m_material(material)
		, m_override_variables(material->m_variables) {
		for (auto& var : m_override_variables) {
			var.second.default_value.valid = false;
		}
	}

	std::shared_ptr<Material> Material::create(Filepath const& path, Pipeline::Description const& pipeline_desc) {
		auto material = std::make_shared<Material>(pipeline_desc);
		material->m_meta.guid = Guid::generate();
		material->m_meta.type = "material";
		material->m_meta.path = path;
		material->m_meta.root = "content";
		auto const& root = FileSystem::s_content_root;
		if (!g_runtime_context.m_asset_manager->register_asset(material->m_meta, root)) {
			return nullptr;
		}
		material->save();
	}

	std::shared_ptr<Material> Material::load(Guid const& guid) {
		auto const& file = g_runtime_context.m_asset_manager->get_file_from_guid(guid);
		if (file.empty()) {
			CORE_ERROR("failed to load material: {0}, file not found!", guid);
			return nullptr;
		}

		YAML::Node node = YAML::LoadFile(file.string());
		auto meta = node["meta"].as<AssetMeta>();
		auto pipeline_node = node["pipeline"];
		Pipeline::Description pipeline_desc{};
		pipeline_desc.depth_test = pipeline_node["depth_test"].as<bool>();
		pipeline_desc.blend = pipeline_node["blend"].as<bool>();
		pipeline_desc.src_blend_factor = static_cast<BlendFactor>(pipeline_node["src_blend_factor"].as<int>());
		pipeline_desc.dst_blend_factor = static_cast<BlendFactor>(pipeline_node["dst_blend_factor"].as<int>());
		pipeline_desc.cull_mode = static_cast<CullMode>(pipeline_node["cull_mode"].as<int>());
		pipeline_desc.shader = g_runtime_context.m_asset_manager->get<Shader>(Guid::make(pipeline_node["shader"].as<std::string>()));
		if (!pipeline_desc.shader) {
			CORE_ERROR("failed to load material: {0}, shader not found!", guid);
			return nullptr;
		}

		return std::make_shared<Material>(pipeline_desc);
	}

	void Material::save() const {
		auto const& root = FileSystem::s_content_root;
		Filepath file = root / m_meta.path;
		file += ".yaml";

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "meta" << YAML::Value << m_meta;
		out << YAML::Key << "pipeline" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "depth_test" << YAML::Value << m_pipeline_desc.depth_test;
		out << YAML::Key << "blend" << YAML::Value << m_pipeline_desc.blend;
		out << YAML::Key << "src_blend_factor" << YAML::Value << static_cast<int>(m_pipeline_desc.src_blend_factor);
		out << YAML::Key << "dst_blend_factor" << YAML::Value << static_cast<int>(m_pipeline_desc.dst_blend_factor);
		out << YAML::Key << "cull_mode" << YAML::Value << static_cast<int>(m_pipeline_desc.cull_mode);
		out << YAML::Key << "shader" << YAML::Value << m_pipeline_desc.shader->m_guid.value;
		out << YAML::EndMap;

		save_yaml(file, out);
	}

}