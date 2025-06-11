#include "pch.h"
#include "render/shader.h"
#include "render/camera.h"
#include "render/framebuffer.h"
#include "render/renderer/renderer_2d.h"
#include "glm/gtc/matrix_transform.hpp"

namespace z1 {

    Renderer2D::Renderer2D() {
#ifdef BATCHED_RENDER
        m_vertex_buffer = VertexBuffer::create(nullptr, s_max_quad_vertices_per_batch * sizeof(QuadVertex),
            {
                {DataType::Float3},
                {DataType::Float2},
                {DataType::Float4},
                {DataType::Float},
                {DataType::Float4},
            }, BufferUsage::Stream);

        auto vertex_indices = new uint32_t[s_max_quad_indices_per_batch];
        uint32_t offset = 0;
        for (uint32_t i = 0; i < s_max_quad_indices_per_batch; i += 6) {
            vertex_indices[i + 0] = offset + 0;
            vertex_indices[i + 1] = offset + 1;
            vertex_indices[i + 2] = offset + 2;

            vertex_indices[i + 3] = offset + 0;
            vertex_indices[i + 4] = offset + 2;
            vertex_indices[i + 5] = offset + 3;

            offset += 4;
        }
        auto index_buffer = IndexBuffer::create(vertex_indices, s_max_quad_indices_per_batch * sizeof(uint32_t), BufferUsage::Static);

        m_vertex_array = VertexArray::create({ m_vertex_buffer }, index_buffer);
#else
        float vertices[] = {
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
             0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
             0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        };

        m_vertex_buffer = VertexBuffer::create(vertices, sizeof(vertices),
            {
                {DataType::Float3},
                {DataType::Float2},
            });
        m_vertex_array = VertexArray::create({ m_vertex_buffer });
#endif

        RenderPass::Description desc;
        desc.m_clear_color = true;
        desc.m_clear_color_value = { 0.1f, 0.1f, 0.1f, 1.0f };
        desc.m_clear_depth = true;
        desc.m_clear_depth_value = 1.0f;

        desc.m_depth_test = true;
        desc.m_blend = true;

        desc.m_framebuffer = nullptr;
#ifdef BATCHED_RENDER
        desc.m_shader = Shader::create(g_runtime_context.m_file_system->m_engine_dir / "asset/shader/sprite_2d_batched.glsl");
#else
        desc.m_shader = Shader::create(g_runtime_context.m_file_system->m_engine_dir / "asset/shader/sprite_2d.glsl");
#endif

        m_render_pass = RenderPass::build(desc);

        uint32_t white = 0xffffffff;
        m_default_texture = Image2D::create(&white, sizeof(white), 1, 1, ImageFormat::RGBA8);
    }

    Renderer2D::~Renderer2D() {

    }

    void Renderer2D::prepare_draw() {
        PROFILE_FUNCTION();

        if (m_quads.empty()) return;

        std::sort(m_quads.begin(), m_quads.end(), [](QuadData const& a, QuadData const& b) {
            if (a.m_model[3][2] == b.m_model[3][2]) {
                return a.m_texture < b.m_texture;
            }
            else {
                return a.m_model[3][2] < b.m_model[3][2];
            }
        });

#ifdef BATCHED_RENDER
        std::vector<glm::vec4> quad_pos = {
            {-0.5f, -0.5f, 0.0f, 1.0f},
            { 0.5f, -0.5f, 0.0f, 1.0f},
            { 0.5f,  0.5f, 0.0f, 1.0f},
            {-0.5f,  0.5f, 0.0f, 1.0f},
        };

        m_quad_vertices.resize(m_quads.size() * 4);
        m_batches.resize(1);
        m_batches[0].m_index_offset = 0;
        m_batches[0].m_vertex_offset = 0;
        uint32_t curr_batch = 0;
        uint32_t vertex_offset = 0;
        uint32_t index_offset = 0;
        for (auto const& quad : m_quads) {
            float texture_id = 0;

            if ((vertex_offset - m_batches[curr_batch].m_vertex_offset) == s_max_quad_vertices_per_batch) {
                m_batches[curr_batch].m_index_num = index_offset - m_batches[curr_batch].m_index_offset;
                m_batches[curr_batch].m_vertex_num = vertex_offset - m_batches[curr_batch].m_vertex_offset;
                m_batches.push_back({});
                curr_batch += 1;
                m_batches[curr_batch].m_index_offset = index_offset;
                m_batches[curr_batch].m_vertex_offset = vertex_offset;
            }

            if (m_batches[curr_batch].m_textures.find(quad.m_texture) != m_batches[curr_batch].m_textures.end()) {
                texture_id = (float)m_batches[curr_batch].m_textures[quad.m_texture];
            }
            else {
                if (m_batches[curr_batch].m_textures.size() == 32) {
                    m_batches[curr_batch].m_index_num = index_offset - m_batches[curr_batch].m_index_offset;
                    m_batches[curr_batch].m_vertex_num = vertex_offset - m_batches[curr_batch].m_vertex_offset;
                    m_batches.push_back({});
                    curr_batch += 1;
                    m_batches[curr_batch].m_index_offset = index_offset;
                    m_batches[curr_batch].m_vertex_offset = vertex_offset;
                }

                texture_id = (float)m_batches[curr_batch].m_textures.size();
                m_batches[curr_batch].m_textures[quad.m_texture] = (uint32_t)texture_id;
            }

            for (int i = 0; i < 4; ++i) {
                auto pos = quad.m_model * quad_pos[i];
                m_quad_vertices[vertex_offset + i].m_position = quad.m_model * quad_pos[i];
                m_quad_vertices[vertex_offset + i].m_texcoord = quad.m_texcoords[i];
                m_quad_vertices[vertex_offset + i].m_color = quad.m_color;
                m_quad_vertices[vertex_offset + i].m_texture_id = texture_id;
                m_quad_vertices[vertex_offset + i].m_tiling_factor = quad.m_tiling_factor;
            }

            vertex_offset += 4;
            index_offset += 6;
        }
        m_batches[curr_batch].m_index_num = index_offset - m_batches[curr_batch].m_index_offset;
        m_batches[curr_batch].m_vertex_num = vertex_offset - m_batches[curr_batch].m_vertex_offset;
#endif
    }

    void Renderer2D::draw() {
        PROFILE_FUNCTION();
        if (!g_runtime_context.m_main_camera) return;

        m_render_pass->bind();
        g_runtime_context.m_main_camera->set_aspect((float)m_render_pass->m_framebuffer->get_description().m_height / (float)m_render_pass->m_framebuffer->get_description().m_width);
        m_render_pass->m_shader->set_uniform("u_projview", &g_runtime_context.m_main_camera->get_projview());
#ifdef BATCHED_RENDER
        for (auto const& batch : m_batches) {
            m_vertex_buffer->write(&m_quad_vertices[batch.m_vertex_offset], batch.m_vertex_num * sizeof(QuadVertex));

            std::vector<int> bindings(32);
            for (auto const& [texture, index] : batch.m_textures) {
                auto texture_binding = g_runtime_context.m_resource_manager->bind_resource(texture->get_resource_id());
                texture->bind(texture_binding);
                bindings[index] = texture_binding;
            }
            m_render_pass->m_shader->set_uniform("u_texture[0]", bindings.data());

            m_vertex_array->bind();
            m_vertex_array->draw(PrimitiveType::Triangles, batch.m_index_num);
            m_vertex_array->unbind();

            for (auto const& [texture, index] : batch.m_textures) {
                g_runtime_context.m_resource_manager->unbind_resource(texture->get_resource_id());
            }
        }
#else
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
            m_render_pass->m_shader->set_uniform("u_tiling_factor", &quad.m_tiling_factor);

            m_vertex_array->bind();
            m_vertex_array->draw(PrimitiveType::Triangles);
            m_vertex_array->unbind();
        }
        if (prev_texture) {
            g_runtime_context.m_resource_manager->unbind_resource(prev_texture->get_resource_id());
        }
#endif
        m_render_pass->unbind();

        m_quads.clear();
    }

    void Renderer2D::draw_quad(
        glm::vec3 const& position,
        glm::vec2 const& size,
        float rotation,
        glm::vec4 const& color) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position)
            * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

        m_quads.push_back({ model, color, m_default_texture });
    }

    void Renderer2D::draw_quad(
        glm::vec3 const& position,
        glm::vec2 const& size,
        float rotation,
        glm::vec4 const& color,
        std::shared_ptr<Image2D> const& texture,
        glm::vec2 const& tiling_scale,
        glm::vec2 const& tiling_offset) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position)
            * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

        m_quads.push_back({ model, color, texture ? texture : m_default_texture, { tiling_scale.x, tiling_scale.y, tiling_offset.x, tiling_offset.y } });
    }

    void Renderer2D::draw_quad(
        glm::vec3 const& position,
        glm::vec2 const& size,
        float rotation,
        glm::vec4 const& color,
        std::shared_ptr<SubImage2D> const& texture) {

        glm::mat4 model = glm::translate(glm::mat4(1.0f), position)
            * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

        QuadData quad = {};
        quad.m_model = model;
        quad.m_color = color;
        if (texture) {
            quad.m_texture = texture->m_image;
            quad.m_texcoords = texture->m_texcoords;
        }
        else {
            quad.m_texture = m_default_texture;
        }

        m_quads.push_back(quad);
    }

    void Renderer2D::draw_quads(std::vector<Quad> const& quads) {
        for (auto const& quad : quads) {
            m_quads.push_back({ quad.m_transform, quad.m_color, quad.m_texture ? quad.m_texture : m_default_texture, { quad.m_tiling_scale.x, quad.m_tiling_scale.y, quad.m_tiling_offset.x, quad.m_tiling_offset.y }, quad.m_texcoords });
        }
    }

}
