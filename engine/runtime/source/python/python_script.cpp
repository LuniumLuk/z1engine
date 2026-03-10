#include "pch.h"
#include "python/python_script.h"
#include "core/log.h"
#include "scene/entity.h"

namespace z1 {

	PythonScript::PythonScript(std::string const& module_name, std::string const& class_name)
		: m_module_name(module_name), m_class_name(class_name) {
	}

	PythonScript::~PythonScript() {
	}

	void PythonScript::on_attach() {
		try {
			auto module = pybind11::module::import(m_module_name.c_str());
			auto cls = module.attr(m_class_name.c_str());
			m_instance = cls();

			if (is_entity_valid()) {
				std::shared_ptr<Entity> entity = m_entity.lock();
				if (entity) {
					m_instance.attr("entity") = pybind11::cast(entity);
				}
			}

			if (pybind11::hasattr(m_instance, "on_attach")) {
				auto func = m_instance.attr("on_attach");
				if (!func.is_none()) func();
			}
		}
		catch (pybind11::error_already_set& e) {
			CORE_ERROR("Python Error (on_attach): {0}", e.what());
		}
	}

	void PythonScript::on_start() {
		if (m_instance.is_none()) return;

		try {
			if (pybind11::hasattr(m_instance, "on_start")) {
				auto func = m_instance.attr("on_start");
				if (!func.is_none()) func();
			}
		}
		catch (pybind11::error_already_set& e) {
			CORE_ERROR("Python Error (on_start): {0}", e.what());
		}
	}

	void PythonScript::on_update(float delta_time) {
		if (m_instance.is_none()) return;

		try {
			if (pybind11::hasattr(m_instance, "on_update")) {
				auto func = m_instance.attr("on_update");
				if (!func.is_none()) func(delta_time);
			}
		}
		catch (pybind11::error_already_set& e) {
			CORE_ERROR("Python Error (on_update): {0}", e.what());
		}
	}

	void PythonScript::on_destroy() {
		if (m_instance.is_none()) return;

		try {
			if (pybind11::hasattr(m_instance, "on_destroy")) {
				auto func = m_instance.attr("on_destroy");
				if (!func.is_none()) func();
			}
		}
		catch (pybind11::error_already_set& e) {
			CORE_ERROR("Python Error (on_destroy): {0}", e.what());
		}
	}

	void PythonScript::on_detach() {
		if (m_instance.is_none()) return;
		// if (!m_instance.ptr()) return; // m_instance.is_none() already checks this

		try {
			if (pybind11::hasattr(m_instance, "on_detach")) {
				auto func = m_instance.attr("on_detach");
				if (!func.is_none()) func();
			}
		}
		catch (pybind11::error_already_set& e) {
			CORE_ERROR("Python Error (on_detach): {0}", e.what());
		}
		m_instance = pybind11::object();
	}

	std::string PythonScript::get_script_name() const {
		return m_module_name + "." + m_class_name;
	}

}
