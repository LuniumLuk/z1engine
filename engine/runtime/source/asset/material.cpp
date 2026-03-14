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

	Material::Material(uint32_t flags, std::shared_ptr<Shader> const& shader)
		: m_flags(flags)
		, m_shader(shader) {

		for (auto& uniform : m_shader->get_uniforms()) {
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
		Filepath path = m_shader->get_path();
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

	std::shared_ptr<Pipeline> Material::get_pipeline(uint32_t flags, uint32_t variant_key) {
		uint64_t pool_key = (uint64_t(variant_key) << 32) | uint64_t(flags);
		if (m_pipeline_pool.find(pool_key) != m_pipeline_pool.end()) {
			return m_pipeline_pool[pool_key];
		}

		// Select the right shader for this variant
		std::shared_ptr<Shader> shader;
		if (variant_key == 0) {
			shader = m_shader;
		}
		else {
			auto it = m_variant_shaders.find(variant_key);
			if (it != m_variant_shaders.end()) {
				shader = it->second;
			}
			else {
				shader = Shader::create(m_shader->get_path(), variant_key);
				m_variant_shaders[variant_key] = shader;
			}
		}

		Pipeline::Description desc{};
		desc.shader = shader;

		desc.depth_test = MaterialFlags::get_depth_test(flags);
		desc.depth_write = MaterialFlags::get_depth_write(flags);
		desc.cull_mode = MaterialFlags::get_cull_mode(flags);

		//desc.blend = (flags & MaterialFlags::BlendMask) != 0;

		auto alpha_mode = MaterialFlags::get_alpha_mode(flags);
		switch (alpha_mode) {
		case AlphaMode::Opaque:
		case AlphaMode::Mask:
			desc.blend = false;
			break;
		case AlphaMode::Blend:
			desc.blend = true;
			desc.src_blend_factor = BlendFactor::SrcAlpha;
			desc.dst_blend_factor = BlendFactor::OneMinusSrcAlpha;
			break;
		}

		auto pipeline = Pipeline::build(desc);
		m_pipeline_pool[pool_key] = pipeline;
		return pipeline;
	}

	void Material::parse_reflection_line(const std::string& line) {
		std::istringstream iss(line);
		std::string token;

		std::string name;
		std::string type;
		std::vector<std::string> params;

		iss >> name;
		if (m_variables.find(name) == m_variables.end()) {
			CORE_WARN("reflected variable: {0} not found in shader {1}!", name, m_shader->get_path());
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
		auto pipeline = m_material->get_pipeline(get_flags(), per_frame.variant_key);
		pipeline->bind();

		auto const& shader = pipeline->m_shader;
		shader->set_uniform_block_binding("Global", per_frame.global_binding);
		if (per_frame.variant_key == 0) {
			// Only bind lighting-related uniforms for forward (non-variant) pass
			shader->set_uniform_block_binding("Lights", per_frame.lights_binding);
		}
		shader->set_uniform("u_model", &per_frame.model);

		for (auto const& [name, var] : m_override_variables) {
			bind_uniform(shader, name);
		}
	}

	uint32_t MaterialInstance::get_flags() const {
		uint32_t flags = m_material->m_flags;
		// Apply overrides
		if (m_override_mask) {
			flags = (flags & ~m_override_mask) | (m_override_flags & m_override_mask);
		}
		return flags;
	}

	void MaterialInstance::bind_uniform(std::shared_ptr<Shader> const& shader, std::string const& material_name) const {
		bind_uniform(shader, material_name, material_name);
	}

	void MaterialInstance::bind_uniform(
		std::shared_ptr<Shader> const& shader,
		std::string const& material_name,
		std::string const& shader_name) const {

		if (!has_uniform(material_name))
			return;

		auto const& var = m_override_variables.at(material_name);
		if (!var.visible || var.location == INVALID_LOCATION)
			return;

		auto* value = &var.default_value;
		if (!value->valid) {
			value = &m_material->m_variables[material_name].default_value;
		}

		switch (var.type) {
		case DataType::Int:
		case DataType::Int2:
		case DataType::Int3:
		case DataType::Int4:
			shader->set_uniform(shader_name, value->ivec);
			break;
		case DataType::Float:
		case DataType::Float2:
		case DataType::Float3:
		case DataType::Float4:
			shader->set_uniform(shader_name, value->vec);
			break;
		case DataType::Sampler2D:
			if (value->tex2D) {
				value->tex2D->m_image->bind(shader, shader_name);
			}
			break;
		default:
			DEBUG_CHECK(false);
			CORE_WARN("unsupported material variable type: {0}", get_data_type_name(var.type));
			break;
		}
	}

#define SET_UNIFORM_COMMON(name)                                \
		if (!has_uniform(name))                                 \
			return;                                             \
		auto& value = m_override_variables[name].default_value; \
		value.valid = true

	void MaterialInstance::set_int(std::string const& name, int val) {
		SET_UNIFORM_COMMON(name);
		value.ivec[0] = val;
	}

	void MaterialInstance::set_ivec2(std::string const& name, glm::ivec2 const& val) {
		SET_UNIFORM_COMMON(name);
		value.ivec[0] = val[0];
		value.ivec[1] = val[1];
	}

	void MaterialInstance::set_ivec3(std::string const& name, glm::ivec3 const& val) {
		SET_UNIFORM_COMMON(name);
		value.ivec[0] = val[0];
		value.ivec[1] = val[1];
		value.ivec[2] = val[2];
	}

	void MaterialInstance::set_ivec4(std::string const& name, glm::ivec4 const& val) {
		SET_UNIFORM_COMMON(name);
		value.ivec[0] = val[0];
		value.ivec[1] = val[1];
		value.ivec[2] = val[2];
		value.ivec[3] = val[3];
	}

	void MaterialInstance::set_float(std::string const& name, float val) {
		SET_UNIFORM_COMMON(name);
		value.vec[0] = val;
	}

	void MaterialInstance::set_vec2(std::string const& name, glm::vec2 const& val) {
		SET_UNIFORM_COMMON(name);
		value.vec[0] = val[0];
		value.vec[1] = val[1];
	}

	void MaterialInstance::set_vec3(std::string const& name, glm::vec3 const& val) {
		SET_UNIFORM_COMMON(name);
		value.vec[0] = val[0];
		value.vec[1] = val[1];
		value.vec[2] = val[2];
	}

	void MaterialInstance::set_vec4(std::string const& name, glm::vec4 const& val) {
		SET_UNIFORM_COMMON(name);
		value.vec[0] = val[0];
		value.vec[1] = val[1];
		value.vec[2] = val[2];
		value.vec[3] = val[3];
	}

	void MaterialInstance::set_texture2d(std::string const& name, std::shared_ptr<Texture2D> const& tex) {
		SET_UNIFORM_COMMON(name);
		value.tex2D = tex;
	}

#undef SET_UNIFORM_COMMON

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
		auto pipeline = m_material->get_pipeline(get_flags());
		pipeline->unbind();
	}

	std::shared_ptr<Material> Material::create(Filepath const& path, uint32_t flags, std::shared_ptr<Shader> const& shader) {
		auto mat = std::make_shared<Material>(flags, shader);
		mat->m_meta.guid = Guid::generate();
		mat->m_meta.type = "material";
		mat->m_meta.path = path;
		auto const& root = FileSystem::s_content_root;
		mat->m_meta.path = g_runtime_context.m_asset_manager->legalize_import_path(mat->m_meta.path);
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

		auto flags = node["flags"].as<uint32_t>();
		auto shader = g_runtime_context.m_asset_manager->get<Shader>(Guid::make(node["shader"].as<std::string>()));
		if (!shader) {
			CORE_ERROR("failed to load material: {0}, shader not found!", guid);
			return nullptr;
		}

		auto mat = std::make_shared<Material>(flags, shader);
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
		out << YAML::Key << "flags" << YAML::Value << m_flags;
		out << YAML::Key << "shader" << YAML::Value << m_shader->m_guid;
		out << YAML::EndMap;

		save_yaml(file, out);
	}

	std::shared_ptr<MaterialInstance> MaterialInstance::create(Filepath const& path, std::shared_ptr<Material> const& material) {
		auto mi = std::make_shared<MaterialInstance>(material);
		mi->m_meta.guid = Guid::generate();
		mi->m_meta.type = "material instance";
		mi->m_meta.path = path;
		auto const& root = FileSystem::s_content_root;
		mi->m_meta.path = g_runtime_context.m_asset_manager->legalize_import_path(mi->m_meta.path);
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

		mi->m_override_flags = node["override_flags"].as<uint32_t>();
		mi->m_override_mask = node["override_mask"].as<uint32_t>();

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
		out << YAML::Key << "override_flags" << YAML::Value << m_override_flags;
		out << YAML::Key << "override_mask" << YAML::Value << m_override_mask;
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
