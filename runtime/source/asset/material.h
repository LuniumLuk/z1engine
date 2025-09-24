#pragma once

#include "core/core.h"
#include "core/guid.h"
#include "asset/asset.h"
#include "asset/texture.h"
#include "render/pipeline.h"
#include "render/resource.h"
#include "render/shader.h"
#include "render/image.h"

namespace z1 {

	struct API PerFrameConst {
		glm::mat4 projview;
		glm::mat4 model;
		glm::vec3 cam_position;
		glm::vec3 sun_direction;
		glm::vec3 sun_intensity;
	};

	struct API Material : Asset<Material> {
		struct Variable {
			std::string name;
			DataType type = DataType::None;
			uint32_t location = INVALID_LOCATION; // location in shader
			uint32_t count = 1;                   // number of elements, for array or vector
			bool visible = true;

			struct Value {
				union {
					std::shared_ptr<Texture2D> tex2D;  // store Texture2D
					int ivec[4];                       // store int, ivec2, ivec3, ivec4
					float vec[4] = { 0.0f };           // store float, vec2, vec3, vec4
				};
				bool valid = false;
				DataType type = DataType::None;

				Value() : tex2D(nullptr), valid(false), type(DataType::None) {}
				Value(Value const& other) : valid(other.valid), type(other.type) {
					switch (other.type) {
					case DataType::Sampler2D:
						tex2D = other.tex2D;
						break;
					default:
						std::memcpy(vec, other.vec, 4 * sizeof(float));
					}
				}
				Value& operator=(Value const& other) {
					if (this != &other) {
						valid = other.valid;
						type = other.type;
						switch (other.type) {
						case DataType::Sampler2D:
							tex2D = other.tex2D;
							break;
						default:
							std::memcpy(vec, other.vec, 4 * sizeof(float));
						}
					}
					return *this;
				}

				~Value() {
					if (type == DataType::Sampler2D) {
						tex2D.reset();
					}
				}
			};

			Value default_value = {};
		};

		static std::shared_ptr<Texture2D> s_default_texture;

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

		void bind(PerFrameConst const& per_frame) const;
		void unbind() const;

		static std::shared_ptr<MaterialInstance> create(Filepath const& path, std::shared_ptr<Material> const& material);
		static std::shared_ptr<MaterialInstance> load(Guid const& guid);
		void save() const;

		std::shared_ptr<Material> m_material;
		std::unordered_map<std::string, Material::Variable> m_override_variables;

	};

}
