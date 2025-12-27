#include "pch.h"

#include "python/python_layer.h"
#include "core/core.h"
#include "core/window.h"
#include "core/application.h"

namespace z1 {

	PythonLayer::PythonLayer() : Layer("Python layer") {

	}

	PythonLayer::~PythonLayer() {
		CORE_DEBUG("shutting down PythonLayer ...");
	}

	void PythonLayer::on_attach() {
	}

	void PythonLayer::on_detach() {
	}

	void PythonLayer::begin() {
		PROFILE_FUNCTION();
	}

	void PythonLayer::end() {
		PROFILE_FUNCTION();
	}

}
