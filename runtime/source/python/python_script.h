#pragma once

#include "scene/component/base.h"
#include <pybind11/embed.h>

namespace z1 {

	struct PythonScript : ScriptBase {
		PythonScript(std::string const& module_name, std::string const& class_name);
		~PythonScript();

		void on_attach() override;
		void on_update(float delta_time) override;
		void on_detach() override;

		std::string get_script_name() const override;

		pybind11::object get_instance() const { return m_instance; }

	private:
		std::string m_module_name;
		std::string m_class_name;
		pybind11::object m_instance;
	};

}
