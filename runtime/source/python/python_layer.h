#pragma once

#include "core/layer.h"

namespace z1 {

	struct API PythonLayer : Layer {
		PythonLayer();
		~PythonLayer();

		void on_attach() override;
		void on_detach() override;

		void begin();
		void end();
	};

}
