#pragma once

#include "render/data_types.h"
#include "core/io.h"
#include "core/guid.h"
#include <string>

namespace z1 {

	struct Image;

	bool file_is_shader(Filepath const& path) noexcept;

	struct API ShaderModule {
		enum struct Stage : int {
			None = 0,
			Geometry,
			Vertex,
			Fragment,
			Compute,
			TessellationControl,
			TessellationEvaluation,
		};

		virtual ~ShaderModule() = default;

		virtual void* get_native_handle() const = 0;
		static std::shared_ptr<ShaderModule> create(Stage stage, std::string const& src);
	};

	struct API Shader {
		struct Variable {
			std::string m_name;
			DataType m_type;
			uint32_t m_count;
			uint32_t m_location;

			Variable(std::string name, DataType type, uint32_t count, uint32_t location)
				: m_name(name)
				, m_type(type)
				, m_count(count)
				, m_location(location) {}
		};

		// A sampler uniform's fixed binding: table slot, GL unit, location, reflected type.
		struct TextureSlot {
			uint32_t m_slot = INVALID_BINDING;
			uint32_t m_unit = INVALID_BINDING;
			uint32_t m_location = INVALID_BINDING;
			DataType m_type = DataType::None;
		};

		// Fast-path handle for a reflected uniform (index into the shader's uniform table).
		struct UniformHandle {
			uint32_t m_index = INVALID_BINDING;
		};

		struct UniformBlock {
			std::string m_name;
			uint32_t m_size;
			uint32_t m_binding;
			std::vector<Variable> m_variables;

			UniformBlock(std::string name, uint32_t size, uint32_t binding, std::vector<Variable> const& variables)
				: m_name(name)
				, m_size(size)
				, m_binding(binding)
				, m_variables(variables) {}
		};

		virtual ~Shader() = default;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual bool has_uniform(std::string const& name) const = 0;
		/*
		* set shader uniform value
		*/
		virtual void set_uniform(std::string const& name, void const* data) = 0;
		/*
		* set shader uniform value by cached handle (no name resolution)
		*/
		virtual void set_uniform(UniformHandle handle, void const* data) = 0;
		/*
		* resolve a uniform handle once; INVALID_BINDING index when the name is unknown
		*/
		virtual UniformHandle uniform_handle(std::string const& name) = 0;
		/*
		* resolve a sampler's fixed slot once (slot == unit); INVALID_BINDING when unknown
		*/
		virtual TextureSlot sampler_slot(std::string const& name) const = 0;
		/*
		* bind a texture (or the type-matched fallback when image is null) to the slot's unit
		*/
		virtual void bind_texture(TextureSlot const& slot, Image const* image) = 0;
		/*
		* bind a texture to one element of a sampler-array slot (unit = slot unit + element)
		*/
		virtual void bind_texture(TextureSlot const& slot, uint32_t element, Image const* image) = 0;
		/*
		* set the binding position of uniform
		* only the opaque uniform types can be set, e.g. samplers, images, atomic counters
		*/
		virtual void set_uniform_binding(std::string const& name, uint32_t binding) = 0;
		/*
		* set the binding position of uniform block
		*/
		virtual void set_uniform_block_binding(std::string const& name, uint32_t binding) = 0;

		/*
		* set binding position of uniform of certain location, deprecate, not recommend to use
		*/
		virtual void set_uniform_binding(uint32_t location, uint32_t binding) = 0;
		/*
		* set binding position of uniform block of certain location, deprecate, not recommend to use
		*/
		virtual void set_uniform_block_binding(uint32_t location, uint32_t binding) = 0;

		/*
		* Get shader uniform value
		*/
		virtual void get_uniform(std::string const& name, void* ptr, size_t size) = 0;
		/*
		* Get shader uniform location
		*/
		virtual uint32_t get_uniform_location(std::string const& name) = 0;
		/*
		* Get shader uniform block location/index
		*/
		virtual uint32_t get_uniform_block_location(std::string const& name) = 0;
		/*
		* Get shader uniform binding position
		* the valid uniform types are consistent with setUniformBinding method
		*/
		virtual uint32_t get_uniform_binding(std::string const& name) = 0;
		/*
		* Get shader uniform block binding position
		*/
		virtual uint32_t get_uniform_block_binding(std::string const& name) = 0;

		std::vector<Variable> const& get_attributes() const { return m_attributes; }
		std::vector<Variable> const& get_uniforms() const { return m_uniforms; }
		std::vector<UniformBlock> const& get_uniform_blocks() const { return m_uniform_blocks; }

		virtual void* get_native_handle() const = 0;
		static std::shared_ptr<Shader> create(Filepath const& path);
		static std::shared_ptr<Shader> create(Filepath const& path, uint32_t variant_key);

		std::string const& get_name() const { return m_name; }
		std::string const& get_path() const { return m_path; }

		Guid m_guid{};

	protected:
		std::vector<Variable> m_attributes;
		std::vector<Variable> m_uniforms;
		std::vector<UniformBlock> m_uniform_blocks;
		std::string m_name;
		std::string m_path;
	};

}
