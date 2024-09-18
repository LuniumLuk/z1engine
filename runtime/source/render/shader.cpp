#include "pch.h"
#include "render/shader.h"
#include "render/rhi/opengl_shader.h"

namespace z1 {

    std::shared_ptr<ShaderModule> ShaderModule::create(Stage stage, std::string const& src) {
        PROFILE_FUNCTION();
        return std::shared_ptr<ShaderModule>(new OpenGLShaderModule(stage, src));
    }

    std::shared_ptr<Shader> Shader::create(Filepath const& path) {
        PROFILE_FUNCTION();
        return std::shared_ptr<Shader>(new OpenGLShader(path));
    }

}
