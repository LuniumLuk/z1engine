#include "render/image.h"
#include "render/buffer.h"
#include "render/vertex_array.h"
#include "render/render_pass.h"

namespace z1 {

    struct API Renderer2D {
        Renderer2D();
        ~Renderer2D();

        void prepare_draw();
        void draw();

        void draw_quad(
            glm::vec3 const& position,
            glm::vec2 const& size,
            float rotation,
            glm::vec4 const& color,
            std::shared_ptr<Image2D> const& texture = nullptr,
            glm::vec2 const& tiling_scale = glm::vec2(1.0f),
            glm::vec2 const& tiling_offset = glm::vec2(0.0f));

    private:

        struct QuadData {
            glm::mat4 m_model;
            glm::vec4 m_color;
            std::shared_ptr<Image2D> m_texture;
            glm::vec4 m_tiling_factor;
        };

        std::vector<QuadData> m_quads;

        std::vector<float> m_quad_vertices;
        std::shared_ptr<VertexArray> m_vertex_array;
        std::shared_ptr<RenderPass> m_render_pass;
        std::shared_ptr<Image2D> m_default_texture;

    };

}
