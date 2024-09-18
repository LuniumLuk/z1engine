#pragma once

#include "render/data_types.h"
#include "core/io.h"
#include <string>

namespace z1 {

    struct API ShaderModule {
        enum struct Stage {
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

        std::vector<Variable> const& GetAttributes() const { return m_attributes; }
        std::vector<Variable> const& GetUniforms() const { return m_uniforms; }
        std::vector<UniformBlock> const& GetUniformBlocks() const { return m_uniform_blocks; }

        virtual void* get_native_handle() const = 0;
        static std::shared_ptr<Shader> create(Filepath const& path);

        std::string const& get_name() const { return m_name; }
        std::string const& get_path() const { return m_path; }

    protected:
        std::vector<Variable> m_attributes;
        std::vector<Variable> m_uniforms;
        std::vector<UniformBlock> m_uniform_blocks;
        std::string m_name;
        std::string m_path;
    };

}
