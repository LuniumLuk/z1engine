#include "pch.h"
#include "render/rhi/opengl_shader.h"
#include "render/shader_variant.h"
#include "util/string_utils.h"
#include "glad/glad.h"

namespace z1 {

	static DataType opengl_type_to_data_type(GLenum type) {
		switch (type) {
		case GL_FLOAT: return DataType::Float;
		case GL_FLOAT_VEC2: return DataType::Float2;
		case GL_FLOAT_VEC3: return DataType::Float3;
		case GL_FLOAT_VEC4: return DataType::Float4;
		case GL_INT: return DataType::Int;
		case GL_INT_VEC2: return DataType::Int2;
		case GL_INT_VEC3: return DataType::Int3;
		case GL_INT_VEC4: return DataType::Int4;
		case GL_FLOAT_MAT3: return DataType::Mat3;
		case GL_FLOAT_MAT4: return DataType::Mat4;
		case GL_BOOL: return DataType::Bool;
		case GL_SAMPLER_2D: return DataType::Sampler2D;
		case GL_SAMPLER_2D_ARRAY: return DataType::Sampler2DArray;
		case GL_SAMPLER_CUBE: return DataType::SamplerCube;
		}
		CORE_ASSERT(false, "unknown data type!");
		return DataType::None;
	}

	static GLenum shader_stage_to_opengl_type(ShaderModule::Stage stage) {
		switch (stage) {
		case ShaderModule::Stage::Geometry: return GL_GEOMETRY_SHADER;
		case ShaderModule::Stage::Vertex: return GL_VERTEX_SHADER;
		case ShaderModule::Stage::Fragment: return GL_FRAGMENT_SHADER;
		case ShaderModule::Stage::Compute: return GL_COMPUTE_SHADER;
		case ShaderModule::Stage::TessellationControl: return GL_TESS_CONTROL_SHADER;
		case ShaderModule::Stage::TessellationEvaluation: return GL_TESS_EVALUATION_SHADER;
		}
		CORE_ASSERT(false, "unknown shader stage!");
		return 0;
	}

	OpenGLShaderModule::OpenGLShaderModule(Stage stage, std::string const& src) {
		PROFILE_FUNCTION();
		m_handle = glCreateShader(shader_stage_to_opengl_type(stage));
		const char* src_data = src.data();
		glShaderSource(m_handle, 1, &src_data, nullptr);
		{
			PROFILE_SCOPE("glCompileShader");
			glCompileShader(m_handle);
		}
		int success;
		char info_log[512];
		glGetShaderiv(m_handle, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(m_handle, 512, nullptr, info_log);
			CORE_ERROR("{}", info_log);
			CORE_ASSERT(false, "failed to compile shader!");
		}
	}

	OpenGLShaderModule::~OpenGLShaderModule() {
		if (m_handle == 0) return;
		glDeleteShader(m_handle);
	}

	OpenGLShader::OpenGLShader(std::initializer_list<OpenGLShaderModule*> shaders) {
		m_handle = glCreateProgram();
		link_shaders(shaders);
	}

	static ShaderModule::Stage str_to_shader_stage(std::string const& stage) {
		if (stage == "vert") return ShaderModule::Stage::Vertex;
		if (stage == "frag") return ShaderModule::Stage::Fragment;
		if (stage == "geom") return ShaderModule::Stage::Geometry;
		if (stage == "comp") return ShaderModule::Stage::Compute;
		if (stage == "tesc") return ShaderModule::Stage::TessellationControl;
		if (stage == "tese") return ShaderModule::Stage::TessellationEvaluation;
		CORE_ASSERT(false, "unknown shader stage!");
		return ShaderModule::Stage::None;
	}

	OpenGLShader::OpenGLShader(Filepath const& path) {
		m_path = path.generic_string();
		m_name = path.filename().generic_string();

		const char* uniform_token = "@uniforms:";
		const size_t uniform_token_len = strlen(uniform_token);
		const char* stage_token = "@stage:";
		const size_t stage_token_len = strlen(stage_token);

		std::vector<OpenGLShaderModule*> shaders;
		std::string uniforms;
		auto code = g_runtime_context.m_file_system->read_file(path);
		size_t pos = 0;

		// find uniforms
		pos = code.find(uniform_token, pos);
		if (pos != std::string::npos) {
			size_t bracket_beg = code.find('{', pos);
			size_t bracket_end = find_paired_brackets(code, bracket_beg);
			uniforms = code.substr(bracket_beg + 1, bracket_end - bracket_beg - 1);
		}

		// find stages
		pos = code.find(stage_token, pos);
		while (pos != std::string::npos) {
			size_t bracket_beg = code.find('{', pos);
			size_t type_beg = pos + stage_token_len;
			auto type = code.substr(type_beg, bracket_beg - type_beg);
			type.erase(std::remove_if(type.begin(), type.end(), ::isspace), type.end());
			size_t bracket_end = find_paired_brackets(code, bracket_beg);
			auto src = code.substr(bracket_beg + 1, bracket_end - bracket_beg - 1);

			src = uniforms + src;
			src = process_includes(src, path.parent_path().generic_string() + "/");
			src = "#version 460 core\n" + src;

			CORE_DEBUG("loading shader stage [{0}] from file {1}", type, path.generic_string());
			shaders.push_back(new OpenGLShaderModule(str_to_shader_stage(type), src));

			pos = code.find(stage_token, bracket_end + 1);
		}

		m_handle = glCreateProgram();
		link_shaders(shaders);

		for (auto shader : shaders) {
			delete shader;
		}
	}

	OpenGLShader::OpenGLShader(Filepath const& path, uint32_t variant_key) {
		m_path = path.generic_string();
		m_name = path.filename().generic_string();

		// Build variant define string from variant_key bits
		std::string variant_defines;
		constexpr uint32_t variant_bits[] = {
			ShaderVariant::GBuffer,
			ShaderVariant::Shadow,
			ShaderVariant::Velocity,
		};
		for (auto bit : variant_bits) {
			if (variant_key & bit) {
				variant_defines += "#define ";
				variant_defines += ShaderVariant::bit_name(bit);
				variant_defines += " 1\n";
			}
		}

		const char* uniform_token = "@uniforms:";
		const size_t uniform_token_len = strlen(uniform_token);
		const char* stage_token = "@stage:";
		const size_t stage_token_len = strlen(stage_token);

		std::vector<OpenGLShaderModule*> shaders;
		std::string uniforms;
		auto code = g_runtime_context.m_file_system->read_file(path);
		size_t pos = 0;

		// find uniforms
		pos = code.find(uniform_token, pos);
		if (pos != std::string::npos) {
			size_t bracket_beg = code.find('{', pos);
			size_t bracket_end = find_paired_brackets(code, bracket_beg);
			uniforms = code.substr(bracket_beg + 1, bracket_end - bracket_beg - 1);
		}

		// Prepend variant defines so they are available in uniform blocks and stage code
		uniforms = variant_defines + uniforms;

		// find stages
		pos = code.find(stage_token, pos);
		while (pos != std::string::npos) {
			size_t bracket_beg = code.find('{', pos);
			size_t type_beg = pos + stage_token_len;
			auto type = code.substr(type_beg, bracket_beg - type_beg);
			type.erase(std::remove_if(type.begin(), type.end(), ::isspace), type.end());
			size_t bracket_end = find_paired_brackets(code, bracket_beg);
			auto src = code.substr(bracket_beg + 1, bracket_end - bracket_beg - 1);

			src = uniforms + src;
			src = process_includes(src, path.parent_path().generic_string() + "/");
			src = "#version 460 core\n" + src;

			CORE_DEBUG("loading shader stage [{0}] variant 0x{1:x} from file {2}", type, variant_key, path.generic_string());
			shaders.push_back(new OpenGLShaderModule(str_to_shader_stage(type), src));

			pos = code.find(stage_token, bracket_end + 1);
		}

		m_handle = glCreateProgram();
		link_shaders(shaders);

		for (auto shader : shaders) {
			delete shader;
		}
	}

	OpenGLShader::~OpenGLShader() {
		if (m_handle == 0) return;
		glDeleteProgram(m_handle);
	}

	void OpenGLShader::bind() const {
		glUseProgram(m_handle);
	}

	void OpenGLShader::unbind() const {
		glUseProgram(0);
	}

	bool OpenGLShader::has_uniform(std::string const& name) const {
		return m_uniform_indices.find(name) != m_uniform_indices.end();
	}

	void OpenGLShader::set_uniform(std::string const& name, void const* data) {
		PROFILE_FUNCTION();
		auto it = m_uniform_indices.find(name);
		if (it == m_uniform_indices.end()) {
			return;
		}
		//DEBUG_CHECK(it != m_uniform_indices.end(), "uniform {0} not found!", name);

		uint32_t index = it->second;
		switch (m_uniforms[index].m_type) {
		case DataType::Bool: set_bool(m_uniforms[index].m_location, *(bool*)data); return;
		case DataType::Int: set_int(m_uniforms[index].m_location, *(int*)data); return;
		case DataType::Float: set_float(m_uniforms[index].m_location, *(float*)data); return;
		case DataType::Float2: set_vec2(m_uniforms[index].m_location, *(glm::vec2*)data); return;
		case DataType::Float3: set_vec3(m_uniforms[index].m_location, *(glm::vec3*)data); return;
		case DataType::Float4: set_vec4(m_uniforms[index].m_location, *(glm::vec4*)data); return;
		case DataType::Mat3: set_mat3(m_uniforms[index].m_location, *(glm::mat3*)data); return;
		case DataType::Mat4: set_mat4(m_uniforms[index].m_location, *(glm::mat4*)data); return;
		case DataType::Sampler2D:
		case DataType::Sampler2DArray:
		case DataType::SamplerCube:
			if (m_uniforms[index].m_count == 1) {
				set_int(m_uniforms[index].m_location, *(int*)data);
			}
			else {
				set_int_array(m_uniforms[index].m_location, (int*)data, m_uniforms[index].m_count);
			}
			return;
		}
		DEBUG_CHECK(false, "uniform {0} with unknown or unsupported DataType!", name);
	}

	void OpenGLShader::set_uniform_binding(std::string const& name, uint32_t binding) {
		PROFILE_FUNCTION();
		auto it = m_uniform_indices.find(name);
		DEBUG_CHECK(it != m_uniform_indices.end(), "uniform {0} not found!", name);

		uint32_t index = it->second;
		switch (m_uniforms[index].m_type) {
		case DataType::Sampler2D:
		case DataType::Sampler2DArray:
		case DataType::SamplerCube:
			set_int(m_uniforms[index].m_location, binding); return;
		}
		DEBUG_CHECK(false, "uniform {0} cannot be set to a binding position!", name);
	}

	void OpenGLShader::set_uniform_block_binding(std::string const& name, uint32_t binding) {
		PROFILE_FUNCTION();
		auto it = m_uniform_block_indices.find(name);
		if (it != m_uniform_block_indices.end()) {
			uint32_t index = it->second;
			glUniformBlockBinding(m_handle, index, binding);
		}
		else {
			CORE_WARN("uniform block {0} not found!", name);
		}
	}

	void OpenGLShader::set_uniform_binding(uint32_t location, uint32_t binding) {
		glUniform1i(location, binding);
	}

	void OpenGLShader::set_uniform_block_binding(uint32_t location, uint32_t binding) {
		glUniformBlockBinding(m_handle, location, binding);
	}

	void OpenGLShader::get_uniform(std::string const& name, void* ptr, size_t size) {
		PROFILE_FUNCTION();
		auto it = m_uniform_indices.find(name);
		if (it != m_uniform_indices.end()) {
			uint32_t index = it->second;
			if (size == get_data_type_size(m_uniforms[index].m_type)) {
				CORE_ASSERT(false, "size not large enough to retrieve the uniform!");
				return;
			}
			switch (m_uniforms[index].m_type) {
			case DataType::Float:
			case DataType::Float2:
			case DataType::Float3:
			case DataType::Float4:
			case DataType::Mat3:
			case DataType::Mat4:
				glGetUniformfv(m_handle, m_uniforms[index].m_location, (GLfloat*)ptr); return;
			case DataType::Int:
			case DataType::Int2:
			case DataType::Int3:
			case DataType::Int4:
			case DataType::Bool:
				glGetUniformiv(m_handle, m_uniforms[index].m_location, (GLint*)ptr); return;
			case DataType::Sampler2D:
			case DataType::Sampler2DArray:
			case DataType::SamplerCube:
				glGetUniformiv(m_handle, m_uniforms[index].m_location, (GLint*)ptr); return;
			}
		}
		else {
			CORE_WARN("uniform block {0} not found!", name);
		}
	}

	uint32_t OpenGLShader::get_uniform_location(std::string const& name) {
		PROFILE_FUNCTION();
		auto it = m_uniform_indices.find(name);
		if (it != m_uniform_indices.end()) {
			uint32_t index = it->second;
			return m_uniforms[index].m_location;
		}
		else {
			CORE_WARN("uniform {0} not found!", name);
			return INVALID_LOCATION;
		}
	}

	uint32_t OpenGLShader::get_uniform_block_location(std::string const& name) {
		PROFILE_FUNCTION();
		auto it = m_uniform_block_indices.find(name);
		if (it != m_uniform_block_indices.end()) {
			return it->second;
		}
		else {
			CORE_WARN("uniform block {0} not found!", name);
			return INVALID_LOCATION;
		}
	}

	uint32_t OpenGLShader::get_uniform_binding(std::string const& name) {
		PROFILE_FUNCTION();
		auto it = m_uniform_indices.find(name);
		uint32_t binding = INVALID_BINDING;
		if (it != m_uniform_indices.end()) {
			uint32_t index = it->second;
			switch (m_uniforms[index].m_type) {
			case DataType::Sampler2D:
			case DataType::Sampler2DArray:
			case DataType::SamplerCube:
				glGetUniformiv(m_handle, m_uniforms[index].m_location, (GLint*)&binding);
			}
		}
		else {
			CORE_WARN("uniform {0} not found!", name);
		}
		return binding;
	}

	uint32_t OpenGLShader::get_uniform_block_binding(std::string const& name) {
		PROFILE_FUNCTION();
		auto it = m_uniform_block_indices.find(name);
		if (it != m_uniform_block_indices.end()) {
			uint32_t index = it->second;
			return m_uniform_blocks[index].m_binding;
		}
		else {
			CORE_WARN("uniform block {0} not found!", name);
			return INVALID_BINDING;
		}
	}

	void OpenGLShader::link_shaders(std::vector<OpenGLShaderModule*> shaders) {
		PROFILE_FUNCTION();
		for (auto shader : shaders) {
			glAttachShader(m_handle, shader->m_handle);
		}
		glLinkProgram(m_handle);
		GLint success;
		GLchar info_log[512];
		glGetProgramiv(m_handle, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(m_handle, 512, NULL, info_log);
			CORE_ERROR("{}", info_log);
			CORE_ASSERT(false, "failed to link shaders!");
		}

		GLint count;
		GLint size;
		GLenum type;
		GLchar name[512];
		GLsizei length;

		glGetProgramiv(m_handle, GL_ACTIVE_ATTRIBUTES, &count);
		for (GLint i = 0; i < count; ++i) {
			glGetActiveAttrib(m_handle, i, 512, &length, &size, &type, name);
			uint32_t location = glGetAttribLocation(m_handle, name);

			auto nameStr = std::string(name);
			m_attribute_indices.insert(std::make_pair(nameStr, (uint32_t)m_attributes.size()));
			m_attributes.emplace_back(nameStr, opengl_type_to_data_type(type), size, location);
		}

		glGetProgramiv(m_handle, GL_ACTIVE_UNIFORMS, &count);
		for (GLint i = 0; i < count; ++i) {
			glGetActiveUniform(m_handle, i, 512, &length, &size, &type, name);
			uint32_t location = glGetUniformLocation(m_handle, name);

			auto nameStr = std::string(name);
			m_uniform_indices.insert(std::make_pair(nameStr, (uint32_t)m_uniforms.size()));
			m_uniforms.emplace_back(nameStr, opengl_type_to_data_type(type), size, location);
		}

		glGetProgramiv(m_handle, GL_ACTIVE_UNIFORM_BLOCKS, &count);
		for (GLint i = 0; i < count; ++i) {
			GLint binding;
			GLint uniformCount;
			glGetActiveUniformBlockiv(m_handle, i, GL_UNIFORM_BLOCK_BINDING, &binding);
			glGetActiveUniformBlockiv(m_handle, i, GL_UNIFORM_BLOCK_DATA_SIZE, &size);
			glGetActiveUniformBlockiv(m_handle, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &uniformCount);

			std::vector<GLint> uniformIndices(uniformCount);
			glGetActiveUniformBlockiv(m_handle, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, uniformIndices.data());
			std::vector<Variable> variables;
			for (auto index : uniformIndices) {
				variables.push_back(m_uniforms[index]);
			}

			glGetActiveUniformBlockName(m_handle, i, 512, &length, name);

			auto nameStr = std::string(name);
			m_uniform_block_indices.insert(std::make_pair(nameStr, (uint32_t)m_uniform_blocks.size()));
			m_uniform_blocks.emplace_back(nameStr, size, binding, variables);
		}
	}

	void OpenGLShader::set_bool(uint32_t location, bool value) {
		glUniform1i(location, (int)value);
	}

	void OpenGLShader::set_int(uint32_t location, int value) {
		glUniform1i(location, value);
	}

	void OpenGLShader::set_float(uint32_t location, float value) {
		glUniform1f(location, value);
	}

	void OpenGLShader::set_vec2(uint32_t location, glm::vec2 const& value) {
		glUniform2fv(location, 1, &value[0]);
	}

	void OpenGLShader::set_vec3(uint32_t location, glm::vec3 const& value) {
		glUniform3fv(location, 1, &value[0]);
	}

	void OpenGLShader::set_vec4(uint32_t location, glm::vec4 const& value) {
		glUniform4fv(location, 1, &value[0]);
	}

	void OpenGLShader::set_mat3(uint32_t location, glm::mat3 const& value) {
		glUniformMatrix3fv(location, 1, GL_FALSE, &value[0][0]);
	}

	void OpenGLShader::set_mat4(uint32_t location, glm::mat4 const& value) {
		glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
	}

	void OpenGLShader::set_int_array(uint32_t location, int* value, int count) {
		glUniform1iv(location, count, value);
	}

	void OpenGLShader::set_float_array(uint32_t location, float* value, int count) {
		glUniform1fv(location, count, value);
	}

}
