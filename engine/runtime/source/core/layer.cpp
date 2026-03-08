#include "pch.h"
#include "core/layer.h"
#include "core/application.h"

namespace z1 {

	Layer::Layer(std::string const& name)
		: m_name(name) {}

	Layer::~Layer() {}

	void Layer::terminate() const {
		m_attached_application->terminate();
	}

}
