#include "z1engine.h"

using namespace z1;

struct PickingSystem {

	PickingSystem(uint32_t w, uint32_t h);

	void render(std::shared_ptr<Scene> const& scene) const;

	uint32_t query(float x, float y) const;

private:
	uint32_t unpack_rgba8_to_uint32(glm::u8vec4 const& rgba) const;

	std::shared_ptr<Framebuffer> m_framebuffer;
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<Pipeline> m_sprite_pipeline;
	std::shared_ptr<RenderPass> m_render_pass;
	std::shared_ptr<VertexArray> m_quad_vao;

};
