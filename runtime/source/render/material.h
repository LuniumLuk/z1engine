#pragma once

#include "core/core.h"
#include "render/render_pass.h"
#include "render/resource.h"
#include "render/shader.h"
#include "render/image.h"

namespace z1 {

    struct API Material {

        struct Uniform {
            std::string m_name;
            union {
                uint32_t m_resource_id;
                void* m_data = nullptr;
            };
            bool m_is_resource = true;

            Uniform(std::string const& name, uint32_t id)
                : m_name(name)
                , m_resource_id(id)
                , m_is_resource(true) {}

            Uniform(std::string const& name, void* data)
                : m_name(name)
                , m_data(data)
                , m_is_resource(false) {}
        };

        void bind(std::shared_ptr<Shader> const& shader) const {
            for (auto const& uniform : m_uniforms) {
                if (uniform->m_is_resource) {
                    switch (g_runtime_context.m_resource_manager->get(uniform->m_resource_id)->get_resource_type()) {
                    case ResourceType::Image:
                        shader->set_uniform_binding(uniform->m_name, g_runtime_context.m_resource_manager->bind_resource(uniform->m_resource_id));
                        break;
                    case ResourceType::UniformBuffer:
                        shader->set_uniform_block_binding(uniform->m_name, g_runtime_context.m_resource_manager->bind_resource(uniform->m_resource_id));
                        break;
                    }
                } 
                else {
                    shader->set_uniform(uniform->m_name, uniform->m_data);
                }
            }
        }

        void unbind() const {
            for (auto const& uniform : m_uniforms) {
                if (uniform->m_is_resource) {
                    g_runtime_context.m_resource_manager->unbind_resource(uniform->m_resource_id);
                }
            }
        }

        /*
        * call register uniform for every uniform (variable, image or uniform buffer)
        * so that when bind() is called, all uniform will be automatically bind
        */
        void register_uniform(Uniform* uniform) {
            m_uniforms.push_back(uniform);
        }

    private:
        std::vector<Uniform*> m_uniforms;

    };

}
