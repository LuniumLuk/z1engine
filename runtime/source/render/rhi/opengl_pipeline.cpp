#include "pch.h"
#include "core/window.h"
#include "render/rhi/opengl_pipeline.h"
#include "render/framebuffer.h"
#include "render/shader.h"
#include "glad/glad.h"

namespace z1 {

	static GLenum blend_factor_to_opengl_type(BlendFactor factor) {
		switch (factor) {
		case BlendFactor::Zero: return GL_ZERO;
		case BlendFactor::One: return GL_ONE;
		case BlendFactor::SrcColor: return GL_SRC_COLOR;
		case BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
		case BlendFactor::DstColor: return GL_DST_COLOR;
		case BlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
		case BlendFactor::SrcAlpha: return GL_SRC_ALPHA;
		case BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DstAlpha: return GL_DST_ALPHA;
		case BlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
		}
		return GL_ZERO;
	}

	static GLenum cull_mode_to_opengl_type(CullMode mode) {
		switch (mode) {
		case CullMode::Front: return GL_FRONT;
		case CullMode::Back: return GL_BACK;
		case CullMode::FrontAndBack: return GL_FRONT_AND_BACK;
		}
		return GL_NONE;
	}

	OpenGLPipeline::OpenGLPipeline(Description const& description) {
		m_shader = description.shader;
		m_setup_func = [this, description] {
			if (description.depth_test) {
				glEnable(GL_DEPTH_TEST);
			}
			else {
				glDisable(GL_DEPTH_TEST);
			}

			if (description.cull_mode == CullMode::None) {
				glDisable(GL_CULL_FACE);
			}
			else {
				glEnable(GL_CULL_FACE);
				glCullFace(cull_mode_to_opengl_type(description.cull_mode));
			}

			if (description.blend) {
				glEnable(GL_BLEND);
				glBlendFunc(
					blend_factor_to_opengl_type(description.src_blend_factor),
					blend_factor_to_opengl_type(description.dst_blend_factor));
			}
			else {
				glDisable(GL_BLEND);
			}
		};
	}

	OpenGLPipeline::~OpenGLPipeline() {

	}

	void OpenGLPipeline::bind() const {
		m_setup_func();
		m_shader->bind();
	}

	void OpenGLPipeline::unbind() const {
		m_shader->unbind();
	}

}
