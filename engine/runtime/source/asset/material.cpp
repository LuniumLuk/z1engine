#include "pch.h"
#include "asset/material.h"
#include "asset/asset_manager.h"
#include "util/string_utils.h"

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
		if (type_str == "sampler2D") return DataType::Sampler2D;
		if (type_str == "sampler2DArray") return DataType::Sampler2DArray;
		if (type_str == "samplerCube") return DataType::SamplerCube;
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
			v.default_value.type = v.type;
			if (v.type == DataType::Mat3 || v.type == DataType::Mat4) {
				// matrices are not supposed to be set as material variables
				// they should be managed by engine internally
				v.visible = false;
			}
			if (v.type == DataType::Sampler2D) {
				v.default_value.tex2D = nullptr;
			}
			m_variables[v.name] = v;
		}

		// parse reflections from shader file
		Filepath path = shader->get_path();
		auto code = g_runtime_context.m_file_system->read_file(path);
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

		reflections = process_includes(reflections, path.parent_path().generic_string() + "/");
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
			CORE_WARN("reflected variable: {0} not found in shader {1}!", name, m_pipeline_desc.shader->get_path());
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
				if (var.type != DataType::Sampler2D && var.type != DataType::SamplerCube && !is_number(token)) {
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
			[[fallthrough]];
		case DataType::Int3:
			var.default_value.ivec[2] = std::stoi(params[2]);
			[[fallthrough]];
		case DataType::Int2:
			var.default_value.ivec[1] = std::stoi(params[1]);
			[[fallthrough]];
		case DataType::Int:
			var.default_value.ivec[0] = std::stoi(params[0]);
			break;
		case DataType::Float4:
			var.default_value.vec[3] = std::stof(params[3]);
			[[fallthrough]];
		case DataType::Float3:
			var.default_value.vec[2] = std::stof(params[2]);
			[[fallthrough]];
		case DataType::Float2:
			var.default_value.vec[1] = std::stof(params[1]);
			[[fallthrough]];
		case DataType::Float:
			var.default_value.vec[0] = std::stof(params[0]);
			break;
		case DataType::Mat3:
		case DataType::Mat4:
			// mat3 and mat4 are not supposed to be set as material variables
			// they should be managed by engine internally
			var.visible = false;
			break;
		case DataType::Sampler2D:
			var.default_value.tex2D = g_runtime_context.m_asset_manager->get<Texture2D>(Guid::make(params[0]));
			break;
		case DataType::SamplerCube:
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

	void MaterialInstance::bind(PerFrameConst const& per_frame) const {
		m_material->m_pipeline->bind();
		auto const& shader = m_material->m_pipeline->m_shader;
		shader->set_uniform_block_binding("Global", per_frame.global_binding);
		shader->set_uniform_block_binding("Lights", per_frame.lights_binding);
		shader->set_uniform("u_model", &per_frame.model);
		for (auto const& [name, var] : m_override_variables) {
			if (!var.visible || var.location == INVALID_LOCATION) continue;

			auto* value = &var.default_value;
			if (!var.default_value.valid) {
				value = &m_material->m_variables[name].default_value;
			}

			switch (var.type) {
			case DataType::Int:
			case DataType::Int2:
			case DataType::Int3:
			case DataType::Int4:
				shader->set_uniform(name, value->ivec);
				break;
			case DataType::Float:
			case DataType::Float2:
			case DataType::Float3:
			case DataType::Float4:
				shader->set_uniform(name, value->vec);
				break;
			case DataType::Sampler2D:
				if (value->tex2D) {
					value->tex2D->m_image->bind(shader, name);
				}
				break;
			default:
				DEBUG_CHECK(false);
				CORE_WARN("unsupported material variable type: {0}", get_data_type_name(var.type));
				break;
			}
		}
	}

	void MaterialInstance::unbind() const {
		for (auto const& [name, var] : m_override_variables) {
			if (!var.visible || var.location == INVALID_LOCATION) continue;

			auto* value = &var.default_value;
			if (!var.default_value.valid) {
				value = &m_material->m_variables[name].default_value;
			}

			if (var.type == DataType::Sampler2D && value->tex2D) {
				value->tex2D->m_image->unbind();
			}
		}
		m_material->m_pipeline->unbind();
	}

	std::shared_ptr<Material> Material::create(Filepath const& path, Pipeline::Description const& pipeline_desc) {
		auto mat = std::make_shared<Material>(pipeline_desc);
		mat->m_meta.guid = Guid::generate();
		mat->m_meta.type = "material";
		mat->m_meta.path = path;
		auto const& root = FileSystem::s_content_root;
		if (!g_runtime_context.m_asset_manager->register_asset(mat->m_meta, root)) {
			return nullptr;
		}
		mat->save();
		return mat;
	}

	std::shared_ptr<Material> Material::load(Guid const& guid) {
		auto file = g_runtime_context.m_asset_manager->get_file_from_guid(guid);
		if (file.empty()) {
			CORE_ERROR("failed to load material: {0}, file not found!", guid);
			return nullptr;
		}

		YAML::Node node = YAML::LoadFile((file.concat(".yaml")).string());
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

		auto mat = std::make_shared<Material>(pipeline_desc);
		mat->m_meta = g_runtime_context.m_asset_manager->get_meta(guid);
		return mat;
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
		out << YAML::Key << "shader" << YAML::Value << m_pipeline_desc.shader->m_guid;
		out << YAML::EndMap;

		save_yaml(file, out);
	}

	std::shared_ptr<MaterialInstance> MaterialInstance::create(Filepath const& path, std::shared_ptr<Material> const& material) {
		auto mi = std::make_shared<MaterialInstance>(material);
		mi->m_meta.guid = Guid::generate();
		mi->m_meta.type = "material instance";
		mi->m_meta.path = path;
		auto const& root = FileSystem::s_content_root;
		if (!g_runtime_context.m_asset_manager->register_asset(mi->m_meta, root)) {
			return nullptr;
		}
		mi->save();
		return mi;
	}

	std::shared_ptr<MaterialInstance> MaterialInstance::load(Guid const& guid) {
		auto file = g_runtime_context.m_asset_manager->get_file_from_guid(guid);
		if (file.empty()) {
			CORE_ERROR("failed to load material: {0}, file not found!", guid);
			return nullptr;
		}

		YAML::Node node = YAML::LoadFile((file.concat(".yaml")).string());
		auto material_guid = Guid::make(node["material"].as<std::string>());
		auto material = g_runtime_context.m_asset_manager->get<Material>(material_guid);
		if (!material) {
			CORE_ERROR("failed to load material instance: {0}, material not found!", guid);
			return nullptr;
		}

		auto mi = std::make_shared<MaterialInstance>(material);
		for (auto const& var_node : node["overrides"]) {
			std::string name = var_node["name"].as<std::string>();
			if (mi->m_override_variables.find(name) == mi->m_override_variables.end()) {
				DEBUG_CHECK(false, "unknown override variable");
				CORE_WARN("material instance: {0} has unknown override variable: {1}", guid, name);
				continue;
			}
			auto& var = mi->m_override_variables[name];
			var.type = static_cast<DataType>(var_node["type"].as<int>());
			if (var.type != material->m_variables[name].type) {
				DEBUG_CHECK(false, "inconsistent type");
				CORE_WARN("material instance: {0} has inconsistent type {1} with material variable {2}!", guid, get_data_type_name(var.type), get_data_type_name(material->m_variables[name].type));
				continue;
			}
			switch (var.type) {
			case DataType::Int4:
				var.default_value.ivec[3] = var_node["value"][3].as<int>();
				/* fallthrough */
			case DataType::Int3:
				var.default_value.ivec[2] = var_node["value"][2].as<int>();
				/* fallthrough */
			case DataType::Int2:
				var.default_value.ivec[1] = var_node["value"][1].as<int>();
				/* fallthrough */
			case DataType::Int:
				var.default_value.ivec[0] = var_node["value"][0].as<int>();
				var.default_value.valid = true;
				break;
			case DataType::Float4:
				var.default_value.vec[3] = var_node["value"][3].as<float>();
				/* fallthrough */
			case DataType::Float3:
				var.default_value.vec[2] = var_node["value"][2].as<float>();
				/* fallthrough */
			case DataType::Float2:
				var.default_value.vec[1] = var_node["value"][1].as<float>();
				/* fallthrough */
			case DataType::Float:
				var.default_value.vec[0] = var_node["value"][0].as<float>();
				var.default_value.valid = true;
				break;
			case DataType::Sampler2D:
				if (var_node["value"] && !var_node["value"].IsNull()) {
					auto tex_guid = Guid::make(var_node["value"].as<std::string>());
					var.default_value.tex2D = g_runtime_context.m_asset_manager->get<Texture2D>(tex_guid);
					if (!var.default_value.tex2D) {
						CORE_WARN("material instance: {0} failed to load texture2D: {1} for variable: {2}", guid, tex_guid.value, name);
					}
					else {
						var.default_value.valid = true;
					}
				}
				break;
			case DataType::SamplerCube:
				break;
			default:
				CORE_WARN("unsupported override variable type: {0}", get_data_type_name(var.type));
				break;
			}
		}

		return mi;
	}

	void MaterialInstance::save() const {
		auto const& root = FileSystem::s_content_root;
		Filepath file = root / m_meta.path;
		file += ".yaml";

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "meta" << YAML::Value << m_meta;
		out << YAML::Key << "material" << YAML::Value << m_material->m_meta.guid;
		out << YAML::Key << "overrides" << YAML::Value;
		out << YAML::BeginSeq;
		for (auto const& [name, var] : m_override_variables) {
			if (!var.default_value.valid) continue;
			out << YAML::BeginMap;
			out << YAML::Key << "name" << YAML::Value << name;
			out << YAML::Key << "type" << YAML::Value << static_cast<int>(var.type);
			switch (var.type) {
			case DataType::Int:
			case DataType::Int2:
			case DataType::Int3:
			case DataType::Int4:
				out << YAML::Key << "value" << YAML::Value;
				out << YAML::Flow << YAML::BeginSeq;
				for (size_t i = 0; i < get_data_type_element_count(var.type); ++i) {
					out << var.default_value.ivec[i];
				}
				out << YAML::EndSeq;
				break;
			case DataType::Float:
			case DataType::Float2:
			case DataType::Float3:
			case DataType::Float4:
				out << YAML::Key << "value" << YAML::Value;
				out << YAML::Flow << YAML::BeginSeq;
				for (size_t i = 0; i < get_data_type_element_count(var.type); ++i) {
					out << var.default_value.vec[i];
				}
				out << YAML::EndSeq;
				break;
			case DataType::Sampler2D:
				if (var.default_value.tex2D) {
					out << YAML::Key << "value" << YAML::Value << var.default_value.tex2D->m_meta.guid;
				}
				else {
					out << YAML::Key << "value" << YAML::Value << YAML::Null;
				}
				break;
			case DataType::SamplerCube:
				break;
			default:
				CORE_WARN("unsupported override variable type: {0}", get_data_type_name(var.type));
				break;
			}
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;

		save_yaml(file, out);
	}

}
