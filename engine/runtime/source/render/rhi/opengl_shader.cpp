#include "pch.h"
#include "render/rhi/opengl_shader.h"
#include "render/shader_variant.h"
#include "render/graphics_context.h"
#include "render/image.h"
#include "render/uniform_blocks.h"
#include "util/prober.h"
#include "util/string_utils.h"
#include "glad/glad.h"

namespace z1 {

#ifdef PLATFORM_MACOS
	static const char* GLSL_VERSION = "#version 410 core\n#define LOCATION(x)\n";
#else
	static const char* GLSL_VERSION = "#version 460 core\n#define LOCATION(x) layout(location = x)\n";
#endif

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
		case GL_SAMPLER_2D_MULTISAMPLE: return DataType::Sampler2DMS;
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
			src = GLSL_VERSION + src;

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
			ShaderVariant::MSAAEdge,
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
			src = GLSL_VERSION + src;

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
		PROBE_COUNT("name_resolutions");
		return m_uniform_indices.find(name) != m_uniform_indices.end();
	}

	void OpenGLShader::set_uniform(std::string const& name, void const* data) {
		PROFILE_FUNCTION();
		PROBE_COUNT("name_resolutions");
		auto it = m_uniform_indices.find(name);
		if (it == m_uniform_indices.end()) {
			return;
		}
		set_uniform_by_index(it->second, data);
	}

	void OpenGLShader::set_uniform(UniformHandle handle, void const* data) {
		if (handle.m_index == INVALID_BINDING || handle.m_index >= (uint32_t)m_uniforms.size()) {
			return;
		}
		set_uniform_by_index(handle.m_index, data);
	}

	Shader::UniformHandle OpenGLShader::uniform_handle(std::string const& name) {
		PROBE_COUNT("name_resolutions");
		auto it = m_uniform_indices.find(name);
		if (it == m_uniform_indices.end()) {
			return {};
		}
		return UniformHandle{ it->second };
	}

	void OpenGLShader::set_uniform_by_index(uint32_t index, void const* data) {
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
		case DataType::Sampler2DMS:
			PROBE_COUNT("sampler_writes");
			PROBE_HASH_MIX(static_cast<uint64_t>(m_handle) | (static_cast<uint64_t>(*(int*)data) << 32));
			if (m_uniforms[index].m_count == 1) {
				set_int(m_uniforms[index].m_location, *(int*)data);
			}
			else {
				set_int_array(m_uniforms[index].m_location, (int*)data, m_uniforms[index].m_count);
			}
			return;
		}
		DEBUG_CHECK(false, "uniform with unknown or unsupported DataType!");
	}

	Shader::TextureSlot OpenGLShader::sampler_slot(std::string const& name) const {
		auto it = m_sampler_slot_indices.find(name);
		if (it == m_sampler_slot_indices.end()) {
			return {};
		}
		SamplerSlotEntry const& entry = m_sampler_slots[it->second];
		TextureSlot slot{};
		slot.m_slot = it->second;
		slot.m_unit = entry.m_unit;
		slot.m_location = entry.m_location;
		slot.m_type = entry.m_type;
		return slot;
	}

	void OpenGLShader::bind_texture(TextureSlot const& slot, Image const* image) {
		if (slot.m_slot == INVALID_BINDING || slot.m_slot >= m_sampler_slots.size()) {
			return;
		}
		SamplerSlotEntry const& entry = m_sampler_slots[slot.m_slot];
		stamp_sampler_slot(slot.m_slot);

		uint32_t handle = 0;
		TextureTarget target = TextureTarget::None;
		switch (entry.m_type) {
		case DataType::Sampler2D: target = TextureTarget::Texture2D; break;
		case DataType::Sampler2DArray: target = TextureTarget::Texture2DArray; break;
		case DataType::SamplerCube: target = TextureTarget::TextureCube; break;
		case DataType::Sampler2DMS: target = TextureTarget::Texture2DMultiSample; break;
		default: return;
		}

		if (image && image->get_native_handle()) {
			handle = static_cast<uint32_t>(reinterpret_cast<uint64_t>(image->get_native_handle()));
			target = image->get_target();
		}
		else {
			handle = g_runtime_context.m_graphics_context->get_fallback_texture(target);
		}
		g_runtime_context.m_graphics_context->bind_texture_unit(entry.m_unit, handle, target);
	}

	void OpenGLShader::bind_texture(TextureSlot const& slot, uint32_t element, Image const* image) {
		if (slot.m_slot == INVALID_BINDING || slot.m_slot >= m_sampler_slots.size()) {
			return;
		}
		SamplerSlotEntry const& entry = m_sampler_slots[slot.m_slot];
		if (element >= entry.m_count) {
			return;
		}
		stamp_sampler_slot(slot.m_slot);

		uint32_t handle = 0;
		TextureTarget target = TextureTarget::Texture2D;
		if (image && image->get_native_handle()) {
			handle = static_cast<uint32_t>(reinterpret_cast<uint64_t>(image->get_native_handle()));
			target = image->get_target();
		}
		else {
			handle = g_runtime_context.m_graphics_context->get_fallback_texture(TextureTarget::Texture2D);
		}
		g_runtime_context.m_graphics_context->bind_texture_unit(entry.m_unit + element, handle, target);
	}

	void OpenGLShader::stamp_sampler_slot(uint32_t slot_index) {
		if (m_slot_stamped[slot_index]) {
			return;
		}
		SamplerSlotEntry const& entry = m_sampler_slots[slot_index];
		if (entry.m_count > 1) {
			std::vector<GLint> units(entry.m_count);
			for (uint32_t i = 0; i < entry.m_count; ++i) {
				units[i] = (GLint)(entry.m_unit + i);
			}
			glProgramUniform1iv(m_handle, (GLint)entry.m_location, (GLsizei)entry.m_count, units.data());
		}
		else {
			glProgramUniform1i(m_handle, (GLint)entry.m_location, (GLint)entry.m_unit);
		}
		m_slot_stamped[slot_index] = 1;
	}

	void OpenGLShader::set_uniform_binding(std::string const& name, uint32_t binding) {
		PROFILE_FUNCTION();
		PROBE_COUNT("name_resolutions");
		PROBE_COUNT("sampler_writes");
		PROBE_HASH_MIX(static_cast<uint64_t>(m_handle) | (static_cast<uint64_t>(binding) << 32));
		auto it = m_uniform_indices.find(name);
		DEBUG_CHECK(it != m_uniform_indices.end(), "uniform {0} not found!", name);

		uint32_t index = it->second;
		switch (m_uniforms[index].m_type) {
		case DataType::Sampler2D:
		case DataType::Sampler2DArray:
		case DataType::SamplerCube:
		case DataType::Sampler2DMS:
			set_int(m_uniforms[index].m_location, binding); return;
		}
		DEBUG_CHECK(false, "uniform {0} cannot be set to a binding position!", name);
	}

	void OpenGLShader::set_uniform_block_binding(std::string const& name, uint32_t binding) {
		PROFILE_FUNCTION();
		PROBE_COUNT("name_resolutions");
		PROBE_COUNT("block_writes");
		PROBE_HASH_MIX(static_cast<uint64_t>(m_handle) | (static_cast<uint64_t>(binding) << 32));
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
		PROBE_COUNT("sampler_writes");
		PROBE_HASH_MIX(static_cast<uint64_t>(m_handle) | (static_cast<uint64_t>(binding) << 32));
		glUniform1i(location, binding);
	}

	void OpenGLShader::set_uniform_block_binding(uint32_t location, uint32_t binding) {
		PROBE_COUNT("block_writes");
		PROBE_HASH_MIX(static_cast<uint64_t>(m_handle) | (static_cast<uint64_t>(binding) << 32));
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
			case DataType::Sampler2DMS:
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
			case DataType::Sampler2DMS:
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

		// Semantic block bindings; unknown blocks are reported instead of silently aliasing Global.
		for (uint32_t i = 0; i < (uint32_t)m_uniform_blocks.size(); ++i) {
			uint32_t const semantic_binding = uniform_blocks::find(m_uniform_blocks[i].m_name.c_str());
			if (semantic_binding == INVALID_BINDING) {
				CORE_ERROR("shader '{0}': uniform block '{1}' is not a semantic block; add it to "
					"render/uniform_blocks.h (Global, Lights, Bones, PrevBones)",
					m_name, m_uniform_blocks[i].m_name);
				continue;
			}
			glUniformBlockBinding(m_handle, i, semantic_binding);
		}

		// Fixed sampler slots: sorted by name, slot == unit, stamped once on first bind.
		std::vector<uint32_t> sampler_uniforms;
		for (uint32_t i = 0; i < (uint32_t)m_uniforms.size(); ++i) {
			DataType const type = m_uniforms[i].m_type;
			if (type == DataType::Sampler2D || type == DataType::Sampler2DArray || type == DataType::SamplerCube || type == DataType::Sampler2DMS) {
				sampler_uniforms.push_back(i);
			}
		}
		std::sort(sampler_uniforms.begin(), sampler_uniforms.end(), [this](uint32_t a, uint32_t b) {
			return m_uniforms[a].m_name < m_uniforms[b].m_name;
		});
		uint32_t unit_cursor = 0;
		for (uint32_t slot = 0; slot < (uint32_t)sampler_uniforms.size(); ++slot) {
			auto const& uniform = m_uniforms[sampler_uniforms[slot]];
			SamplerSlotEntry entry{};
			entry.m_location = uniform.m_location;
			entry.m_unit = unit_cursor;
			entry.m_count = uniform.m_count > 0 ? uniform.m_count : 1;
			entry.m_type = uniform.m_type;
			m_sampler_slot_indices.emplace(uniform.m_name, slot);
			m_sampler_slots.push_back(entry);
			unit_cursor += entry.m_count;
		}
		m_slot_stamped.assign(m_sampler_slots.size(), 0);

		uint32_t const max_units = g_runtime_context.m_graphics_context->m_max_fragment_texture_units;
		if (max_units > 0 && unit_cursor > max_units) {
			CORE_ERROR("shader '{0}': {1} sampler units exceed the per-stage limit ({2}); reduce sampler count",
				m_name, unit_cursor, max_units);
		}

		// Default every sampler so programs that never set one stay valid until the binder stamps it.
		GLint const default_binding = (GLint)g_runtime_context.m_graphics_context->m_default_sampler_binding;
		glUseProgram(m_handle);
		for (auto const& uniform : m_uniforms) {
			if (uniform.m_type == DataType::Sampler2D ||
				uniform.m_type == DataType::Sampler2DArray ||
				uniform.m_type == DataType::SamplerCube ||
				uniform.m_type == DataType::Sampler2DMS) {
				glUniform1i(uniform.m_location, default_binding);
			}
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
