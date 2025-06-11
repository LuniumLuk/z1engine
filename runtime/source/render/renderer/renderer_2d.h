#include "render/image.h"
#include "render/buffer.h"
#include "render/vertex_array.h"
#include "render/render_pass.h"

namespace z1 {

#define BATCHED_RENDER

	struct API Renderer2D {
#ifdef BATCHED_RENDER
		static const uint32_t s_max_quads_per_batch = 2048;
		static const uint32_t s_max_quad_vertices_per_batch = s_max_quads_per_batch * 4;
		static const uint32_t s_max_quad_indices_per_batch = s_max_quads_per_batch * 6;
#endif

		struct Quad {
			glm::mat4 m_transform = glm::mat4(1.0f);
			glm::vec4 m_color = glm::vec4(1.0f);
			std::shared_ptr<Image2D> m_texture = nullptr;
			glm::vec2 m_tiling_scale = glm::vec2(1.0f);
			glm::vec2 m_tiling_offset = glm::vec2(0.0f);
			std::array<glm::vec2, 4> m_texcoords = { {
				{ 0.0f, 0.0f },
				{ 1.0f, 0.0f },
				{ 1.0f, 1.0f },
				{ 0.0f, 1.0f },
			} };
		};

		Renderer2D();
		~Renderer2D();

		void prepare_draw();
		void draw();

		void draw_quad(
			glm::vec3 const& position,
			glm::vec2 const& size,
			float rotation,
			glm::vec4 const& color);

		void draw_quad(
			glm::vec3 const& position,
			glm::vec2 const& size,
			float rotation,
			glm::vec4 const& color,
			std::shared_ptr<Image2D> const& texture,
			glm::vec2 const& tiling_scale = glm::vec2(1.0f),
			glm::vec2 const& tiling_offset = glm::vec2(0.0f));

		void draw_quad(
			glm::vec3 const& position,
			glm::vec2 const& size,
			float rotation,
			glm::vec4 const& color,
			std::shared_ptr<SubImage2D> const& texture);

		void draw_quads(std::vector<Quad> const& quads);

	private:

		struct QuadData {
			glm::mat4 m_model;
			glm::vec4 m_color;
			std::shared_ptr<Image2D> m_texture;
			glm::vec4 m_tiling_factor = { 1.0f, 1.0f, 0.0f, 0.0f };
			std::array<glm::vec2, 4> m_texcoords = { {
				{ 0.0f, 0.0f },
				{ 1.0f, 0.0f },
				{ 1.0f, 1.0f },
				{ 0.0f, 1.0f },
			} };
		};
		std::vector<QuadData> m_quads;

#ifdef BATCHED_RENDER
		struct QuadVertex {
			glm::vec3 m_position;
			glm::vec2 m_texcoord;
			glm::vec4 m_color;
			float m_texture_id;
			glm::vec4 m_tiling_factor;
		};
		std::vector<QuadVertex> m_quad_vertices;

		struct BatchData {
			std::unordered_map<std::shared_ptr<Image2D>, uint32_t> m_textures;
			uint32_t m_index_offset;
			uint32_t m_index_num;
			uint32_t m_vertex_offset;
			uint32_t m_vertex_num;
		};
		std::vector<BatchData> m_batches;
#endif

		std::shared_ptr<VertexBuffer> m_vertex_buffer;
		std::shared_ptr<VertexArray> m_vertex_array;
		std::shared_ptr<RenderPass> m_render_pass;
		std::shared_ptr<Image2D> m_default_texture;

	};

}
