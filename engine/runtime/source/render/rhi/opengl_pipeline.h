#pragma once

#include "render/pipeline.h"
#include <functional>

namespace z1 {

	struct OpenGLPipeline : Pipeline {
		OpenGLPipeline(Description const& description);
		~OpenGLPipeline();

		void bind() const override;
		void unbind() const override;

	private:
		std::function<void()> m_setup_func;
	};

}
