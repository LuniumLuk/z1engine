#pragma once

#include "core/core.h"
#include "core/guid.h"
#include "asset/asset.h"
#include "asset/texture.h"
#include "render/pipeline.h"
#include "render/resource.h"
#include "render/shader.h"
#include "render/shader_variant.h"
#include "render/image.h"

namespace z1 {

	struct API PerFrameConst {
		glm::mat4 model;
		uint32_t global_binding = INVALID_BINDING;
		uint32_t lights_binding = INVALID_BINDING;
		uint32_t shadow_map_binding = INVALID_BINDING;
		uint32_t variant_key = 0;
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
		constexpr uint32_t BlendMask = 0x1 << 6;    // 0=Off, 1=On (SrcAlpha, OneMinusSrcAlpha)

		constexpr uint32_t Default = 0; // Opaque, no depth test, no depth write, back cull, no blend
		constexpr uint32_t Scene = DepthTest | DepthWrite | (static_cast<uint32_t>(CullMode::Back) << 4);

		inline AlphaMode get_alpha_mode(uint32_t flags) {
			return static_cast<AlphaMode>(flags & AlphaModeMask);
		}
		inline bool get_depth_test(uint32_t flags) {
			return (flags & DepthTest) != 0;
		}
		inline bool get_depth_write(uint32_t flags) {
			return (flags & DepthWrite) != 0;
		}
		inline CullMode get_cull_mode(uint32_t flags) {
			return static_cast<CullMode>((flags & CullModeMask) >> 4);
		}
		inline bool get_blend(uint32_t flags) {
			return (flags & BlendMask) != 0;
		}

		inline void set_alpha_mode(uint32_t& flags, AlphaMode mode) {
			flags &= ~AlphaModeMask; // Clear existing alpha mode bits
			flags |= (static_cast<uint32_t>(mode) & AlphaModeMask); // Set new alpha mode
		}
		inline void set_depth_test(uint32_t& flags, bool enable) {
			if (enable) {
				flags |= DepthTest;
			}
			else {
				flags &= ~DepthTest;
			}
		}
		inline void set_depth_write(uint32_t& flags, bool enable) {
			if (enable) {
				flags |= DepthWrite;
			}
			else {
				flags &= ~DepthWrite;
			}
		}
		inline void set_cull_mode(uint32_t& flags, CullMode mode) {
			flags &= ~CullModeMask; // Clear existing cull mode bits
			flags |= (static_cast<uint32_t>(mode) << 4) & CullModeMask; // Set new cull mode
		}
		inline void set_blend(uint32_t& flags, bool enable) {
			if (enable) {
				flags |= BlendMask;
			}
			else {
				flags &= ~BlendMask;
			}
		}
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

		Material(uint32_t flags, Guid const& shader_guid);

		static std::shared_ptr<Material> create(Filepath const& path, uint32_t flags, Guid const& shader_guid);
		static std::shared_ptr<Material> load(Guid const& guid);
		void save() const;

		uint32_t m_flags = 0;

		Guid m_shader_guid;
		std::unordered_map<std::string, Variable> m_variables;

		std::unordered_map<uint64_t, std::shared_ptr<Pipeline>> m_pipeline_pool;
		std::unordered_map<uint32_t, std::shared_ptr<Shader>> m_variant_shaders;
		std::shared_ptr<Shader> get_shader(uint32_t variant_key = 0);
		std::shared_ptr<Pipeline> get_pipeline(uint32_t flags, uint32_t variant_key = 0);

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

		std::shared_ptr<Pipeline> get_pipeline(uint32_t variant_key = 0) const { return m_material->get_pipeline(get_flags(), variant_key); }
		std::shared_ptr<Shader> get_shader(uint32_t variant_key = 0) const { return m_material->get_shader(variant_key); }

		std::shared_ptr<Material> m_material;
		std::unordered_map<std::string, Material::Variable> m_override_variables;

		uint32_t m_override_flags = 0;
		uint32_t m_override_mask = 0;

		uint32_t get_flags() const;

		bool has_uniform(std::string const& name) const {
			return m_material->m_variables.find(name) != m_material->m_variables.end();
		}

		void bind_uniform(std::shared_ptr<Shader> const& shader, std::string const& material_name) const;
		void bind_uniform(std::shared_ptr<Shader> const& shader, std::string const& material_name, std::string const& shader_name) const;

		void set_int(std::string const& name, int val);
		void set_ivec2(std::string const& name, glm::ivec2 const& val);
		void set_ivec3(std::string const& name, glm::ivec3 const& val);
		void set_ivec4(std::string const& name, glm::ivec4 const& val);

		void set_float(std::string const& name, float val);
		void set_vec2(std::string const& name, glm::vec2 const& val);
		void set_vec3(std::string const& name, glm::vec3 const& val);
		void set_vec4(std::string const& name, glm::vec4 const& val);

		void set_texture2d(std::string const& name, std::shared_ptr<Texture2D> const& tex);

	};

}
