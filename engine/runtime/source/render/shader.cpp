#include "pch.h"
#include "render/shader.h"
#include "render/rhi/opengl_shader.h"
#include "render/shader_variant.h"

namespace z1 {

	bool file_is_shader(Filepath const& path) noexcept {
		auto ext = path.extension().string();
		const std::vector<std::string> imageExtensions = { ".glsl" };
		return std::find(imageExtensions.begin(), imageExtensions.end(), ext) != imageExtensions.end();
	}

	std::shared_ptr<ShaderModule> ShaderModule::create(Stage stage, std::string const& src) {
		PROFILE_FUNCTION();
		return std::shared_ptr<ShaderModule>(new OpenGLShaderModule(stage, src));
	}

	std::shared_ptr<Shader> Shader::create(Filepath const& path) {
		PROFILE_FUNCTION();
		return std::shared_ptr<Shader>(new OpenGLShader(path));
	}

	std::shared_ptr<Shader> Shader::create(Filepath const& path, uint32_t variant_key) {
		PROFILE_FUNCTION();
		return std::shared_ptr<Shader>(new OpenGLShader(path, variant_key));
	}

}
