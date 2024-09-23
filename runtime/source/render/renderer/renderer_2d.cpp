#include "pch.h"
#include "render/shader.h"
#include "render/camera.h"
#include "render/framebuffer.h"
#include "render/renderer/renderer_2d.h"
#include "glm/gtc/matrix_transform.hpp"

namespace z1 {

    Renderer2D::Renderer2D() {
        float vertices[] = {
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
             0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
             0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        };

        m_vertex_array = VertexArray::create({
            VertexBuffer::create(vertices, sizeof(vertices),
                {
                    {DataType::Float3},
                    {DataType::Float2},
                })
            });

        RenderPass::Description desc;
        desc.m_clear_color = true;
        desc.m_clear_color_value = { 0.1f, 0.1f, 0.1f, 1.0f };
        desc.m_clear_depth = true;
        desc.m_clear_depth_value = 1.0f;

        desc.m_depth_test = true;
        desc.m_blend = true;

        desc.m_framebuffer = nullptr;
        desc.m_shader = Shader::create(g_runtime_context.m_file_system->m_engine_dir / "asset/shader/sprite_2d.glsl");

        m_render_pass = RenderPass::build(desc);

        uint32_t white = 0xffffffff;
        m_default_texture = Image2D::create(&white, sizeof(white), 1, 1, ImageFormat::RGBA8);
    }

    Renderer2D::~Renderer2D() {

    }

    void Renderer2D::prepare_draw() {
        std::sort(m_quads.begin(), m_quads.end(), [](QuadData const& a, QuadData const& b) {
            return a.m_model[3][2] > b.m_model[3][2];
            //return a.m_texture < b.m_texture;
        });
    }

    void Renderer2D::draw() {
        if (!g_runtime_context.m_main_camera) return;


        m_render_pass->bind();
        g_runtime_context.m_main_camera->set_aspect((float)m_render_pass->m_framebuffer->get_description().m_height / (float)m_render_pass->m_framebuffer->get_description().m_width);
        m_render_pass->m_shader->set_uniform("u_projview", &g_runtime_context.m_main_camera->get_projview());
        std::shared_ptr<Image2D> prev_texture = nullptr;
        for (auto const& quad : m_quads) {
            if (quad.m_texture != prev_texture) {
                if (prev_texture) {
                    g_runtime_context.m_resource_manager->unbind_resource(prev_texture->get_resource_id());
                }

                auto texture_binding = g_runtime_context.m_resource_manager->bind_resource(quad.m_texture->get_resource_id());
                quad.m_texture->bind(texture_binding);
                m_render_pass->m_shader->set_uniform_binding("u_texture", texture_binding);

                prev_texture = quad.m_texture;
            }

            m_render_pass->m_shader->set_uniform("u_model", &quad.m_model);
            m_render_pass->m_shader->set_uniform("u_color", &quad.m_color);

            m_vertex_array->bind();
            m_vertex_array->draw(PrimitiveType::Triangles);
            m_vertex_array->unbind();
        }
        if (prev_texture) {
            g_runtime_context.m_resource_manager->unbind_resource(prev_texture->get_resource_id());
        }
        m_render_pass->unbind();

        m_quads.clear();
    }

    void Renderer2D::draw_quad(glm::vec3 const& position, glm::vec2 const& size, glm::vec4 const& color, std::shared_ptr<Image2D> const& texture) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, position);
        model = glm::scale(model, glm::vec3(size, 1.0f));

        m_quads.push_back({ model, color, texture ? texture : m_default_texture });
    }

}
