#include "pch.h"
#include "render/resource.h"
#include "render/graphics_context.h"

namespace z1 {

	Resource::Resource(ResourceType type)
		: m_type(type)
		, m_id(g_runtime_context.m_resource_manager->register_resource(this)) {
		CORE_INFO("resource created, type: {0}, id: {1}", get_resource_name(m_type), m_id);
	}

	Resource::~Resource() {
		g_runtime_context.m_resource_manager->unregister_resource(m_id);
		CORE_INFO("resource destroyed, type: {0}, id: {1}", get_resource_name(m_type), m_id);
	}

	ResourceManager::ResourceManager() {
		uint32_t max_image_binding = g_runtime_context.m_graphics_context->m_max_image_binding_count;
		for (uint32_t i = 0; i < max_image_binding; ++i) {
			m_valid_image_bindings.push(max_image_binding - 1 - i);
		}

		uint32_t max_uniform_buffer_binding = g_runtime_context.m_graphics_context->m_max_uniform_buffer_binding_count;
		for (uint32_t i = 0; i < max_uniform_buffer_binding; ++i) {
			m_valid_uniform_buffer_bindings.push(max_uniform_buffer_binding - 1 - i);
		}

		CORE_INFO("init resource manager with max image bindings: {0}", max_image_binding);
		CORE_INFO("init resource manager with max uniform buffer bindings: {0}", max_uniform_buffer_binding);
	}

	ResourceManager::~ResourceManager() {

	}

	uint32_t ResourceManager::bind_resource(uint32_t id) {
		if (id >= m_resources.size()) {
			CORE_ERROR("resource id {0} is out of range", id);
			return INVALID_BINDING;
		}

		if (!m_resources[id]) {
			CORE_ERROR("resource id {0} has been destroyed", id);
			return INVALID_BINDING;
		}

		m_resources[id]->m_ref_count += 1;

		if (m_resources[id]->is_bound()) {
			return m_resources[id]->get_binding();
		}

		uint32_t binding = INVALID_BINDING;
		switch (m_resources[id]->get_resource_type()) {
		case ResourceType::Image:
			if (m_valid_image_bindings.empty()) {
				CORE_WARN("no more image binding point available");
				return INVALID_BINDING;
			}
			binding = m_valid_image_bindings.top();
			m_valid_image_bindings.pop();
			break;
		case ResourceType::UniformBuffer:
			if (m_valid_uniform_buffer_bindings.empty()) {
				CORE_WARN("no more uniform buffer binding point available");
				return INVALID_BINDING;
			}
			binding = m_valid_uniform_buffer_bindings.top();
			m_valid_uniform_buffer_bindings.pop();
			break;
		}

		m_resources[id]->bind(binding);
		m_resources[id]->m_binding = binding;
		return binding;
	}

	void ResourceManager::unbind_resource(uint32_t id) {
		if (id >= m_resources.size()) {
			CORE_ERROR("resource id {0} is out of range", id);
			return;
		}

		if (!m_resources[id]) {
			CORE_ERROR("resource id {0} has been destroyed", id);
			return;
		}

		m_resources[id]->m_ref_count -= 1;
		if (m_resources[id]->m_ref_count > 0) {
			return;
		}

		m_resources[id]->unbind(m_resources[id]->m_binding);
		switch (m_resources[id]->m_type) {
		case ResourceType::Image:
			m_valid_image_bindings.push(m_resources[id]->m_binding); break;
		case ResourceType::UniformBuffer:
			m_valid_uniform_buffer_bindings.push(m_resources[id]->m_binding); break;
		}
		m_resources[id]->m_binding = INVALID_BINDING;
	}

}
