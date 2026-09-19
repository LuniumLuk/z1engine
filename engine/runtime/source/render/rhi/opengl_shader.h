#pragma once

#include "render/shader.h"
#include "core/core.h"
#include "core/io.h"
#include "glm/glm.hpp"

namespace z1 {

	struct OpenGLShaderModule : ShaderModule {
		friend struct OpenGLShader;

		OpenGLShaderModule(Stage stage, std::string const& src);
		~OpenGLShaderModule() override;

		void* get_native_handle() const override { return (void*)(uint64_t)m_handle; }

	private:
		uint32_t m_handle = 0;
	};

	struct OpenGLShader : Shader {
		OpenGLShader(std::initializer_list<OpenGLShaderModule*> shaders);
		OpenGLShader(Filepath const& path);
		OpenGLShader(Filepath const& path, uint32_t variant_key);
		~OpenGLShader() override;

		void bind() const override;
		void unbind() const override;

		bool has_uniform(std::string const& name) const override;
		void set_uniform(std::string const& name, void const* data) override;
		void set_uniform(UniformHandle handle, void const* data) override;
		UniformHandle uniform_handle(std::string const& name) override;
		TextureSlot sampler_slot(std::string const& name) const override;
		void bind_texture(TextureSlot const& slot, Image const* image) override;
		void bind_texture(TextureSlot const& slot, uint32_t element, Image const* image) override;
		void set_uniform_binding(std::string const& name, uint32_t binding) override;
		void set_uniform_block_binding(std::string const& name, uint32_t binding) override;

		void set_uniform_binding(uint32_t location, uint32_t binding) override;
		void set_uniform_block_binding(uint32_t location, uint32_t binding) override;

		void get_uniform(std::string const& name, void* ptr, size_t size) override;
		uint32_t get_uniform_location(std::string const& name) override;
		uint32_t get_uniform_block_location(std::string const& name) override;
		uint32_t get_uniform_binding(std::string const& name) override;
		uint32_t get_uniform_block_binding(std::string const& name) override;

		void* get_native_handle() const override { return (void*)(uint64_t)m_handle; }

	private:
		uint32_t m_handle = 0;
		std::unordered_map<std::string, uint32_t> m_attribute_indices;
		std::unordered_map<std::string, uint32_t> m_uniform_indices;
		std::unordered_map<std::string, uint32_t> m_uniform_block_indices;

		// Fixed sampler slots (slot == unit): built at link, stamped once on first bind.
		struct SamplerSlotEntry {
			uint32_t m_location = 0;
			uint32_t m_unit = 0;
			uint32_t m_count = 1;
			DataType m_type = DataType::None;
		};
		std::vector<SamplerSlotEntry> m_sampler_slots;
		std::unordered_map<std::string, uint32_t> m_sampler_slot_indices;
		std::vector<uint8_t> m_slot_stamped;

		void stamp_sampler_slot(uint32_t slot_index);
		void set_uniform_by_index(uint32_t index, void const* data);
		void link_shaders(std::vector<OpenGLShaderModule*> shaders);

		void set_bool(uint32_t location, bool value);
		void set_int(uint32_t location, int value);
		void set_float(uint32_t location, float value);
		void set_vec2(uint32_t location, glm::vec2 const& value);
		void set_vec3(uint32_t location, glm::vec3 const& value);
		void set_vec4(uint32_t location, glm::vec4 const& value);
		void set_mat3(uint32_t location, glm::mat3 const& value);
		void set_mat4(uint32_t location, glm::mat4 const& value);
		void set_int_array(uint32_t location, int* value, int count);
		void set_float_array(uint32_t location, float* value, int count);
	};

}
