#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <regex>
#include <filesystem>
#include <map>

// Include GLAD and GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#define PLATFORM_MACOS
#endif

// Macros replacement - with better formatting
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"

#define CORE_ERROR(...) { printf(COLOR_RED "[ERROR] " COLOR_RESET); printf(__VA_ARGS__); printf("\n"); }
#define CORE_WARN(...)  { printf(COLOR_YELLOW "[WARN]  " COLOR_RESET); printf(__VA_ARGS__); printf("\n"); }
#define CORE_INFO(...)  { printf(COLOR_CYAN "[INFO]  " COLOR_RESET); printf(__VA_ARGS__); printf("\n"); }
#define CORE_SUCCESS(...) { printf(COLOR_GREEN COLOR_BOLD "[OK]    " COLOR_RESET); printf(__VA_ARGS__); printf("\n"); }
#define PROFILE_FUNCTION()
#define PROFILE_SCOPE(x)

// Namespace alias
namespace fs = std::filesystem;

// Error collection
struct ErrorInfo {
	std::string file;
	std::string variant;
	std::string stage;
	std::string message;
};

static std::vector<ErrorInfo> g_errors;

// Add error to collection
void add_error(const std::string& file, const std::string& variant, const std::string& stage, const std::string& message) {
	g_errors.push_back({file, variant, stage, message});
}

// Print all collected errors in a formatted summary
void print_error_summary() {
	if (g_errors.empty()) {
		return;
	}

	printf("\n");
	printf("====================================================\n");
	printf("                  COMPILATION ERRORS               \n");
	printf("====================================================\n");
	printf("\n");

	// Group errors by file
	std::map<std::string, std::vector<ErrorInfo>> errors_by_file;
	for (const auto& err : g_errors) {
		errors_by_file[err.file].push_back(err);
	}

	int error_count = 1;
	for (const auto& [file, errors] : errors_by_file) {
		printf(COLOR_RED "[FILE] %s" COLOR_RESET "\n", file.c_str());
		for (const auto& err : errors) {
			printf("  " COLOR_RED "[Error %d]" COLOR_RESET, error_count++);
			if (!err.variant.empty()) {
				printf(" Variant: %s", err.variant.c_str());
			}
			if (!err.stage.empty()) {
				printf(" | Stage: %s", err.stage.c_str());
			}
			printf("\n");
			printf("    %s\n", err.message.c_str());
			printf("\n");
		}
	}

	printf("Total errors: %zu\n\n", g_errors.size());
}

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

bool compile_shader_module(Stage stage, std::string const& src, GLuint& out_handle, const std::string& file = "", const std::string& variant = "", const std::string& stage_name = "") {
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

		// Collect error
		if (!file.empty()) {
			add_error(file, variant, stage_name, info_log);
		}

		glDeleteShader(out_handle);
		return false;
	}
	return true;
}

bool link_program(const std::vector<GLuint>& shaders, GLuint& out_program, const std::string& file = "", const std::string& variant = "") {
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

		// Collect error
		if (!file.empty()) {
			add_error(file, variant, "linking", info_log);
		}

		glDeleteProgram(out_program);
		return false;
	}
	return true;
}

bool validate_shader_file(fs::path const& path) {
	printf("\n");
	CORE_INFO("Validating shader: %s", path.string().c_str());
	printf("  [FILE] %s\n", path.filename().string().c_str());

	std::string code = read_file(path);
	if (code.empty()) {
		CORE_ERROR("Failed to read file or file is empty");
		return false;
	}

	const char* uniform_token = "@uniforms:";
	const char* stage_token = "@stage:";
	const char* variant_token = "@variants:";
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

	// find variants
	std::vector<std::string> variant_names;
	pos = code.find(variant_token, 0);
	if (pos != std::string::npos) {
		size_t bracket_beg = code.find('{', pos);
		size_t bracket_end = find_paired_brackets(code, bracket_beg);
		if (bracket_beg != std::string::npos && bracket_end != std::string::npos) {
			std::string variants_block = code.substr(bracket_beg + 1, bracket_end - bracket_beg - 1);
			std::istringstream vstream(variants_block);
			std::string vline;
			while (std::getline(vstream, vline)) {
				// trim whitespace
				vline.erase(0, vline.find_first_not_of(" \t\r\n"));
				vline.erase(vline.find_last_not_of(" \t\r\n") + 1);
				if (!vline.empty()) {
					variant_names.push_back(vline);
				}
			}
		}
	}

	// Enumerate all variant combinations: 0 .. (2^N - 1)
	size_t num_variants = variant_names.size();
	size_t num_combos = 1u << num_variants; // 2^N

	if (num_variants > 0) {
		printf("  [VARIANTS] %zu (testing %zu combinations)\n", num_variants, num_combos);
		printf("    Variant names: ");
		for (size_t i = 0; i < variant_names.size(); ++i) {
			if (i > 0) printf(", ");
			printf("%s", variant_names[i].c_str());
		}
		printf("\n");
	}

	bool all_success = true;

	for (size_t combo = 0; combo < num_combos; ++combo) {
		// Build variant defines string for this combination
		std::string variant_defines;
		std::string combo_label = "base";
		if (combo != 0) {
			combo_label = "";
			for (size_t bit = 0; bit < num_variants; ++bit) {
				if (combo & (1u << bit)) {
					variant_defines += "#define " + variant_names[bit] + " 1\n";
					if (!combo_label.empty()) combo_label += "+";
					combo_label += variant_names[bit];
				}
			}
		}

		printf("  [%zu/%zu] Testing variant: %s\n", combo + 1, num_combos, combo_label.c_str());

		// Prepend variant defines to uniforms
		std::string full_uniforms = variant_defines + uniforms;

		std::vector<GLuint> shader_handles;
		bool combo_success = true;

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
				combo_success = false;
				break;
			}

			auto src = code.substr(bracket_beg + 1, bracket_end - bracket_beg - 1);
			src = full_uniforms + src;
			src = process_includes(src, path.parent_path().string() + "/");
#ifdef PLATFORM_MACOS
			src = "#version 410 core\n#define LOCATION(x)\n" + src;
#else
			src = "#version 460 core\n#define LOCATION(x) layout(location = x)\n" + src;
#endif

			Stage stage = str_to_shader_stage(type);
			if (stage == Stage::None) {
				CORE_ERROR("Unknown stage type: %s", type.c_str());
				combo_success = false;
			}
			else {
				GLuint handle = 0;
				if (compile_shader_module(stage, src, handle, path.filename().string(), combo_label, type)) {
					printf("      [OK] %s shader compiled successfully\n", type.c_str());
					shader_handles.push_back(handle);
				}
				else {
					CORE_ERROR("  Failed to compile %s stage for variant: %s", type.c_str(), combo_label.c_str());
					combo_success = false;
				}
			}

			pos = code.find(stage_token, bracket_end + 1);
		}

		if (shader_handles.empty()) {
			CORE_WARN("No shader stages found in %s", path.string().c_str());
			for (auto h : shader_handles) glDeleteShader(h);
			return false;
		}

		GLuint program = 0;
		if (combo_success) {
			if (link_program(shader_handles, program, path.filename().string(), combo_label)) {
				printf("      [OK] Program linked successfully\n");
			}
			else {
				CORE_ERROR("  Failed to link variant: %s", combo_label.c_str());
				combo_success = false;
			}
		}

		// Cleanup
		for (auto h : shader_handles) glDeleteShader(h);
		if (program) glDeleteProgram(program);

		if (!combo_success) {
			all_success = false;
		}
	}

	if (all_success) {
		if (num_variants > 0) {
			CORE_SUCCESS("All %zu variant combinations validated successfully!", num_combos);
		}
		else {
			CORE_SUCCESS("Shader validated successfully!");
		}
	}

	return all_success;
}

// ----------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------
int main(int argc, char** argv) {
	// Initialize GLFW (headless if possible, but we need context)
	if (!glfwInit()) {
		CORE_ERROR("Failed to initialize GLFW");
		return -1;
	}

	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#ifdef PLATFORM_MACOS
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
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

	bool success = true;
	if (argc < 2) {
		printf("\n");
		printf("====================================================\n");
		printf("     SHADER VALIDATOR - Batch Mode                  \n");
		printf("====================================================\n");
		printf("Scanning all shader files under engine/content/shader/\n");
		printf("\n");

		int total_files = 0;
		int failed_files = 0;

		fs::path shader_dir = "engine/content/shader/";
		fs::path exclude_dir = "include";
		for (const auto& entry : fs::recursive_directory_iterator(shader_dir, fs::directory_options::skip_permission_denied)) {
			if (entry.is_regular_file() && entry.path().extension() == ".glsl" && entry.path().string().find(exclude_dir.string()) == std::string::npos) {
				total_files++;
				bool file_success = validate_shader_file(entry.path());
				if (!file_success) failed_files++;
				success = success && file_success;
			}
		}

		printf("\n");
		printf("====================================================\n");
		printf("                   VALIDATION SUMMARY               \n");
		printf("====================================================\n");
		printf("Total files:   %d\n", total_files);
		printf("Failed files:  %d\n", failed_files);
		printf("Passed files:  %d\n", total_files - failed_files);
		if (success) {
			CORE_SUCCESS("All shader files validated successfully!");
		}
		else {
			CORE_ERROR("Some shader files failed validation");
		}
		printf("\n");

		// Print detailed errors at the end
		print_error_summary();
	}
	else {
		printf("\n");
		printf("====================================================\n");
		printf("     SHADER VALIDATOR - Single File Mode            \n");
		printf("====================================================\n");
		success = validate_shader_file(argv[1]);
		printf("\n");

		// Print detailed errors at the end
		print_error_summary();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return success ? 0 : 1;
}
