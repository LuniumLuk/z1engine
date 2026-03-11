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
		//glm::mat4 projview;
		glm::mat4 model;
		//glm::vec3 cam_position;
		//glm::vec3 sun_direction;
		//glm::vec3 sun_intensity;
		uint32_t global_binding;
		uint32_t lights_binding;
		uint32_t shadow_map_binding;
	};

	enum class AlphaMode : uint32_t {
		Opaque = 0,
		Mask = 1,
		Blend = 2
	};

	namespace MaterialFlags {
		constexpr uint32_t AlphaModeMask = 0x3;
		constexpr uint32_t DepthTest = 1 << 2;
		constexpr uint32_t DepthWrite = 1 << 3;
		constexpr uint32_t CullModeMask = 0x3 << 4; // 0=None, 1=Front, 2=Back
		constexpr uint32_t BlendMask = 0x1 << 6;     // 0=Off, 1=On (SrcAlpha, OneMinusSrcAlpha)
	}

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

		Material(Pipeline::Description const& pipeline_desc);

		static std::shared_ptr<Material> create(Filepath const& path, Pipeline::Description const& pipeline_desc);
		static std::shared_ptr<Material> load(Guid const& guid);
		void save() const;

		Pipeline::Description m_pipeline_desc;
		std::unordered_map<std::string, Variable> m_variables;

		// Pipeline pooling
		std::shared_ptr<Pipeline> m_pipeline; // Default pipeline
		std::unordered_map<uint32_t, std::shared_ptr<Pipeline>> m_pipeline_pool;
		uint32_t m_flags = 0;

		AlphaMode m_alpha_mode = AlphaMode::Opaque;
		float m_alpha_cutoff = 0.5f;
		bool m_double_sided = false;

		std::shared_ptr<Pipeline> get_pipeline(uint32_t flags);
		static uint32_t get_flags_from_pipeline_desc(Pipeline::Description const& desc, AlphaMode alpha_mode, bool double_sided);
		static void apply_flags_to_pipeline_desc(Pipeline::Description& desc, uint32_t flags);

	private:
		void parse_reflection_line(const std::string& line);

	};

	struct API MaterialInstance : Asset<MaterialInstance> {

		MaterialInstance(std::shared_ptr<Material> const& material);

		void bind(PerFrameConst const& per_frame) const;
		void bind_uniforms(std::shared_ptr<Shader> const& shader, PerFrameConst const& per_frame) const;
		void unbind() const;

		static std::shared_ptr<MaterialInstance> create(Filepath const& path, std::shared_ptr<Material> const& material);
		static std::shared_ptr<MaterialInstance> load(Guid const& guid);
		void save() const;

		std::shared_ptr<Material> m_material;
		std::unordered_map<std::string, Material::Variable> m_override_variables;

		uint32_t m_override_flags = 0;
		uint32_t m_override_mask = 0;

		uint32_t get_flags() const;
		void set_flag(uint32_t flag, bool enable);
		void set_alpha_mode(AlphaMode mode);
	};

}
