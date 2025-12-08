#include "pch.h"
#include "render/resource.h"

namespace z1 {

	RenderResource::RenderResource(ResourceType type)
		: m_type(type)
		, m_id(generate_id()) {
		CORE_DEBUG("resource created, type: {0}, id: {1}", get_type_name(), m_id);
	}

	RenderResource::~RenderResource() {
		CORE_DEBUG("resource destroyed, type: {0}, id: {1}", get_type_name(), m_id);
	}

}
