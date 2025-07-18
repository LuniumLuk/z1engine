#pragma once

#include "core/core.h"
#include "render/pipeline.h"
#include "render/resource.h"
#include "render/shader.h"
#include "render/image.h"

namespace z1 {

	struct API Material {
		struct Variable {
			std::string name;
			DataType type = DataType::None;
			uint32_t location = INVALID_LOCATION; // location in shader
			uint32_t count = 1;                   // number of elements, for array or vector
			bool visible = true;

			struct Value {
				union {
					uint32_t resource_id;    // store Image2D and ImageCube
					int ivec[4];             // store int, ivec2, ivec3, ivec4
					float vec[4] = { 0.0f }; // store float, vec2, vec3, vec4
				};
				bool valid = false;
			};

			Value default_value = {};
		};

		Material(std::string const& name, std::shared_ptr<Pipeline> const& pipeline);

		std::string m_name;
		std::shared_ptr<Pipeline> m_pipeline;
		std::unordered_map<std::string, Variable> m_variables;

	private:
		void parse_reflection_line(const std::string& line);
	};

	struct API MaterialInstance {

		MaterialInstance(std::string const& name, std::shared_ptr<Material> const& material);

		std::string m_name;
		std::shared_ptr<Material> m_material;
		std::unordered_map<std::string, Material::Variable> m_override_variables;

	};

}
