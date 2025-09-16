#pragma once

#include "core/core.h"
#include "core/guid.h"
#include "asset/asset.h"
#include "render/pipeline.h"
#include "render/resource.h"
#include "render/shader.h"
#include "render/image.h"

namespace z1 {

	struct API Material : Asset<Material> {
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

		Material(Pipeline::Description const& pipeline_desc);

		static std::shared_ptr<Material> create(Filepath const& path, Pipeline::Description const& pipeline_desc);
		static std::shared_ptr<Material> load(Guid const& guid);
		void save() const;

		Pipeline::Description m_pipeline_desc;
		std::unordered_map<std::string, Variable> m_variables;
		std::shared_ptr<Pipeline> m_pipeline;

	private:
		void parse_reflection_line(const std::string& line);

	};

	struct API MaterialInstance : Asset<MaterialInstance> {

		MaterialInstance(std::shared_ptr<Material> const& material);

		static std::shared_ptr<MaterialInstance> create(Filepath const& path, std::shared_ptr<Material> const& material);
		static std::shared_ptr<MaterialInstance> load(Guid const& guid);
		void save() const;

		std::shared_ptr<Material> m_material;
		std::unordered_map<std::string, Material::Variable> m_override_variables;

	};

}
