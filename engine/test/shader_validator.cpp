#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <regex>
#include <filesystem>

// Include GLAD and GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Macros replacement
#define CORE_ERROR(...) { printf("ERROR: "); printf(__VA_ARGS__); printf("\n"); }
#define CORE_WARN(...)  { printf("WARN:  "); printf(__VA_ARGS__); printf("\n"); }
#define CORE_INFO(...)  { printf("INFO:  "); printf(__VA_ARGS__); printf("\n"); }
#define PROFILE_FUNCTION()
#define PROFILE_SCOPE(x)

// Namespace alias
namespace fs = std::filesystem;

// ----------------------------------------------------------------------------
// Utilities from string_utils.h
// ----------------------------------------------------------------------------
size_t find_paired_brackets(std::string const& code, size_t start) {
	size_t pos = code.find('{', start);
	size_t counter = 1;
	while (counter > 0 && pos != std::string::npos) {
		pos += 1;
		if (code[pos] == '{') {
			counter += 1;
		}
		else if (code[pos] == '}') {
			counter -= 1;
		}
	}
	return pos;
}

std::string process_includes(const std::string& input, const std::string& search_dir) {
	std::string result;
	std::istringstream input_stream(input);
	std::string line;
	std::regex include_regex(R"(^\s*#include\s*<([^>]+)>\s*$)");

	while (std::getline(input_stream, line)) {
		std::smatch match;
		if (std::regex_match(line, match, include_regex)) {
			std::string filename = match[1].str();
			std::ifstream file(search_dir + filename);

			if (file.is_open()) {
				std::stringstream file_stream;
				file_stream << file.rdbuf();
				file.close();
				result += file_stream.str() + "\n";
			}
			else {
				CORE_ERROR("failed to open included file: %s", (search_dir + filename).c_str());
				result += line + "\n";
			}
		}
		else {
			result += line + "\n";
		}
	}
	return result;
}

std::string read_file(fs::path const& path) {
	std::ifstream in(path, std::ios::in | std::ios::binary);
	if (in) {
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return contents;
	}
	CORE_ERROR("Failed to read file: %s", path.string().c_str());
	return "";
}

// ----------------------------------------------------------------------------
// Shader Compilation Logic
// ----------------------------------------------------------------------------

enum class Stage {
	None = 0,
	Geometry,
	Vertex,
	Fragment,
	Compute,
	TessellationControl,
	TessellationEvaluation,
};

static GLenum shader_stage_to_opengl_type(Stage stage) {
	switch (stage) {
	case Stage::Geometry: return GL_GEOMETRY_SHADER;
	case Stage::Vertex: return GL_VERTEX_SHADER;
	case Stage::Fragment: return GL_FRAGMENT_SHADER;
	case Stage::Compute: return GL_COMPUTE_SHADER;
	case Stage::TessellationControl: return GL_TESS_CONTROL_SHADER;
	case Stage::TessellationEvaluation: return GL_TESS_EVALUATION_SHADER;
	}
	return 0;
}

static Stage str_to_shader_stage(std::string const& stage) {
	if (stage == "vert") return Stage::Vertex;
	if (stage == "frag") return Stage::Fragment;
	if (stage == "geom") return Stage::Geometry;
	if (stage == "comp") return Stage::Compute;
	if (stage == "tesc") return Stage::TessellationControl;
	if (stage == "tese") return Stage::TessellationEvaluation;
	return Stage::None;
}

bool compile_shader_module(Stage stage, std::string const& src, GLuint& out_handle) {
	out_handle = glCreateShader(shader_stage_to_opengl_type(stage));
	const char* src_data = src.data();
	glShaderSource(out_handle, 1, &src_data, nullptr);
	glCompileShader(out_handle);

	int success;
	char info_log[2048];
	glGetShaderiv(out_handle, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(out_handle, 2048, nullptr, info_log);
		CORE_ERROR("Shader Compilation Failed:\n%s", info_log);
		glDeleteShader(out_handle);
		return false;
	}
	return true;
}

bool link_program(const std::vector<GLuint>& shaders, GLuint& out_program) {
	out_program = glCreateProgram();
	for (auto shader : shaders) {
		glAttachShader(out_program, shader);
	}
	glLinkProgram(out_program);

	GLint success;
	GLchar info_log[2048];
	glGetProgramiv(out_program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(out_program, 2048, NULL, info_log);
		CORE_ERROR("Program Linking Failed:\n%s", info_log);
		glDeleteProgram(out_program);
		return false;
	}
	return true;
}

bool validate_shader_file(fs::path const& path) {
	CORE_INFO("Validating: %s", path.string().c_str());

	std::string code = read_file(path);
	if (code.empty()) return false;

	const char* uniform_token = "@uniforms:";
	const char* stage_token = "@stage:";
	const size_t stage_token_len = strlen(stage_token);

	std::string uniforms;
	size_t pos = 0;

	// find uniforms
	pos = code.find(uniform_token, pos);
	if (pos != std::string::npos) {
		size_t bracket_beg = code.find('{', pos);
		size_t bracket_end = find_paired_brackets(code, bracket_beg);
		if (bracket_beg != std::string::npos && bracket_end != std::string::npos) {
			uniforms = code.substr(bracket_beg + 1, bracket_end - bracket_beg - 1);
		}
	}

	std::vector<GLuint> shader_handles;
	bool all_success = true;

	// find stages
	pos = 0;
	pos = code.find(stage_token, pos);
	while (pos != std::string::npos) {
		size_t bracket_beg = code.find('{', pos);
		size_t type_beg = pos + stage_token_len;
		auto type = code.substr(type_beg, bracket_beg - type_beg);
		// trim whitespace
		type.erase(std::remove_if(type.begin(), type.end(), ::isspace), type.end());

		size_t bracket_end = find_paired_brackets(code, bracket_beg);
		if (bracket_beg == std::string::npos || bracket_end == std::string::npos) {
			CORE_ERROR("Malformed brackets for stage %s", type.c_str());
			all_success = false;
			break;
		}

		auto src = code.substr(bracket_beg + 1, bracket_end - bracket_beg - 1);
		src = uniforms + src;
		src = process_includes(src, path.parent_path().string() + "/");

		Stage stage = str_to_shader_stage(type);
		if (stage == Stage::None) {
			CORE_ERROR("Unknown stage type: %s", type.c_str());
			all_success = false;
		} else {
			CORE_INFO("Compiling stage: %s", type.c_str());
			GLuint handle = 0;
			if (compile_shader_module(stage, src, handle)) {
				shader_handles.push_back(handle);
			} else {
				all_success = false;
			}
		}

		pos = code.find(stage_token, bracket_end + 1);
	}

	if (shader_handles.empty()) {
		CORE_WARN("No shader stages found in %s", path.string().c_str());
		return false;
	}

	GLuint program = 0;
	if (all_success) {
		if (link_program(shader_handles, program)) {
			CORE_INFO("SUCCESS: Shader validated successfully.");
		} else {
			all_success = false;
		}
	}

	// Cleanup
	for (auto h : shader_handles) glDeleteShader(h);
	if (program) glDeleteProgram(program);

	return all_success;
}

// ----------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------
int main(int argc, char** argv) {
	if (argc < 2) {
		std::cout << "Usage: shader_validator <path_to_shader_file>" << std::endl;
		return 1;
	}

	// Initialize GLFW (headless if possible, but we need context)
	if (!glfwInit()) {
		CORE_ERROR("Failed to initialize GLFW");
		return -1;
	}

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(640, 480, "ShaderValidator", NULL, NULL);
	if (!window) {
		CORE_ERROR("Failed to create GLFW window");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		CORE_ERROR("Failed to initialize GLAD");
		glfwTerminate();
		return -1;
	}

	bool success = validate_shader_file(argv[1]);

	glfwDestroyWindow(window);
	glfwTerminate();

	return success ? 0 : 1;
}
