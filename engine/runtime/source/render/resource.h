#pragma once

#include "core/core.h"
#include "core/io.h"
#include <stack>

namespace z1 {

#define RESOURCE_TYPES \
	X(Image)           \
	X(Buffer)          \
	X(Framebuffer)     \
	X(Pipeline)        \
	X(VertexArray)     \

#define X(name) name,
	enum struct API ResourceType {
		RESOURCE_TYPES
	};
#undef X

#define X(name) case ResourceType::name: return #name;
	inline std::string get_resource_name(ResourceType type) {
		switch (type) {
			RESOURCE_TYPES
		}
		return "unknown resource type";
	}
#undef X

	struct API RenderResource {

		RenderResource(ResourceType type);
		virtual ~RenderResource();

		void set_name(std::string const& name) { m_name = name; }
		std::string const& get_name() const { return m_name; }
		ResourceType get_type() const { return m_type; }
		std::string get_type_name() const { return get_resource_name(m_type); }
		uint32_t get_id() const { return m_id; }

	private:
		static uint32_t generate_id() {
			static uint32_t s_id_counter = 0;
			return s_id_counter++;
		}

		std::string m_name;
		ResourceType m_type;
		uint32_t m_id;
	};

}
