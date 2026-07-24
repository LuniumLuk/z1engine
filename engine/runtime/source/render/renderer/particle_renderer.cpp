#include "pch.h"
#include "render/renderer/particle_renderer.h"
#include "render/shader.h"
#include "render/buffer.h"
#include "render/vertex_array.h"
#include "render/global.h"
#include "scene/component/particle.h"
#include "scene/component/base.h"
#include "scene/component/camera.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "asset/asset_manager.h"

namespace z1 {

	void ParticleRenderer::init() {
		// Load particle shader
		auto particle_shader = g_runtime_context.m_asset_manager->get<Shader>("$engine/shader/particle");

		// Data-driven pipeline construction table
		struct PipelineEntry {
			std::shared_ptr<Pipeline>* target;
			BlendFactor src;
			BlendFactor dst;
		};

		PipelineEntry const entries[] = {
			{ &m_pipeline_alpha,    BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha },
			{ &m_pipeline_additive, BlendFactor::SrcAlpha, BlendFactor::One },
			{ &m_pipeline_soft,     BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha },
		};

		for (auto const& e : entries) {
			Pipeline::Description desc;
			desc.blend = true;
			desc.depth_test = true;
			desc.depth_write = false;
			desc.cull_mode = CullMode::None;
			desc.src_blend_factor = e.src;
			desc.dst_blend_factor = e.dst;
			desc.shader = particle_shader;
			*e.target = Pipeline::build(desc);
		}

		// Create shared unit quad geometry
		// 4 vertices: quad_offset [-1,1] + texcoord [0,1]
		ParticleQuadVertex quad_verts[] = {
			{ {-1.0f, -1.0f}, {0.0f, 0.0f} },  // bottom-left
			{ { 1.0f, -1.0f}, {1.0f, 0.0f} },  // bottom-right
			{ { 1.0f,  1.0f}, {1.0f, 1.0f} },  // top-right
			{ {-1.0f,  1.0f}, {0.0f, 1.0f} },  // top-left
		};

		m_quad_vbo = VertexBuffer::create(quad_verts, sizeof(quad_verts),
			{
				{DataType::Float2},  // quad_offset
				{DataType::Float2},  // texcoord
			}, BufferUsage::Static);

		uint32_t quad_indices[] = { 0, 1, 2, 2, 3, 0 };
		m_quad_ibo = IndexBuffer::create(quad_indices, sizeof(quad_indices), BufferUsage::Static);

		m_quad_vao = VertexArray::create({ m_quad_vbo }, m_quad_ibo);

		// Shadow pipeline (depth-only, no blend)
		{
			Pipeline::Description desc;
			desc.depth_test = true;
			desc.depth_write = true;
			desc.cull_mode = CullMode::None;
			desc.shader = g_runtime_context.m_asset_manager->get<Shader>("$engine/shader/particle_shadow");
			m_pipeline_shadow = Pipeline::build(desc);
		}
	}

	void ParticleRenderer::shutdown() {
		m_pipeline_alpha.reset();
		m_pipeline_additive.reset();
		m_pipeline_soft.reset();
		m_pipeline_shadow.reset();
		m_quad_vbo.reset();
		m_quad_ibo.reset();
		m_quad_vao.reset();
	}

	std::shared_ptr<Pipeline> ParticleRenderer::select_pipeline(ParticleBlendMode mode) {
		switch (mode) {
		case ParticleBlendMode::Alpha:
			return m_pipeline_alpha;
		case ParticleBlendMode::Additive:
			return m_pipeline_additive;
		case ParticleBlendMode::Soft:
			return m_pipeline_soft;
		default:
			return m_pipeline_additive;
		}
	}

	void ParticleRenderer::add_particle_pass(RenderGraph& rg, Scene* scene, std::string const& input_pass, std::shared_ptr<Image> const& shadow_image) {
		if (!scene) return;

		rg.add_pass("particles")
			.set_passthrough(input_pass)
			.execute([this, scene, shadow_image](RenderGraphNode& node, GraphicsContext& ctx) {
				auto view = scene->m_registry.view<ParticleComponent, TransformComponent const>();

				// Find depth attachment image from passthrough framebuffer (once per frame)
				std::shared_ptr<Image> depth_image;
				auto const& fb = node.get_output();
				if (fb) {
					auto const& attachments = fb->get_attachments();
					for (uint32_t i = 0; i < attachments.size(); ++i) {
						if (attachments[i].format == ImageFormat::Depth || attachments[i].format == ImageFormat::DepthStencil) {
							depth_image = fb->get_attachment_image(i);
							break;
						}
					}
				}

				for (auto entity : view) {
					auto& pc = view.get<ParticleComponent>(entity);
					auto& transform = view.get<TransformComponent>(entity);

					// Skip if no alive particles
					if (pc.m_runtime.m_alive_count == 0) continue;

					// Get camera
					auto cam_entity = scene->get_main_camera();
					if (!cam_entity) continue;
					auto& camera_comp = cam_entity->get_component<CameraComponent>();

					// Initialize or resize per-component instance buffer if needed
					size_t required_size = static_cast<size_t>(pc.m_max_particles) * sizeof(ParticleInstanceData);
					if (!pc.m_runtime.m_vbo || pc.m_runtime.m_vbo->get_size() < required_size) {
						pc.m_runtime.m_vbo = VertexBuffer::create(nullptr, required_size,
							{
								{DataType::Float3},  // position
								{DataType::Float},   // size
								{DataType::Float4},  // color
								{DataType::Float},   // rotation
							}, BufferUsage::Stream);
					}

					// Build instance data from alive particles (1 entry per particle)
					std::vector<ParticleInstanceData> instances;
					instances.reserve(pc.m_runtime.m_alive_count);

					for (auto const& particle : pc.m_runtime.m_particles) {
						if (!particle.alive) continue;

						ParticleInstanceData inst;
						if (pc.m_world_space) {
							inst.position = particle.position;
						}
						else {
							inst.position = glm::vec3(transform.get_world_transform() * glm::vec4(particle.position, 1.0f));
						}
						inst.size = particle.size;
						inst.color = particle.color;
						inst.rotation = particle.rotation;
						instances.push_back(inst);
					}

					if (instances.empty()) continue;

					// Upload instance data to GPU
					pc.m_runtime.m_vbo->write(instances.data(), instances.size() * sizeof(ParticleInstanceData));

					// Select correct pipeline based on blend mode
					auto pipeline = select_pipeline(pc.m_blend_mode);
					if (!pipeline) continue;

					pipeline->bind();

					// Bind Global UBO so shadow sampling uniforms (u_sun_projview, u_csm_splits, u_cam_position) are available
					pipeline->m_shader->set_uniform_block_binding("Global", g_runtime_context.m_global->get_binding());

					// Set shader uniforms
					auto view_mat = camera_comp.get_view();
					auto projview = camera_comp.get_proj() * view_mat;
					pipeline->m_shader->set_uniform("u_projview", &projview);

					// Extract camera right/up from view matrix (rows 0 and 1)
					glm::vec3 cam_right = glm::vec3(view_mat[0][0], view_mat[1][0], view_mat[2][0]);
					glm::vec3 cam_up    = glm::vec3(view_mat[0][1], view_mat[1][1], view_mat[2][1]);
					pipeline->m_shader->set_uniform("u_cam_right", &cam_right);
					pipeline->m_shader->set_uniform("u_cam_up", &cam_up);

					// Bind texture if available
					int has_texture = 0;
					if (pc.m_texture && pc.m_texture->m_image) {
						pc.m_texture->m_image->bind();
						int binding = (int)pc.m_texture->m_image->get_binding();
						pipeline->m_shader->set_uniform("u_texture", &binding);
						has_texture = 1;
					}
					pipeline->m_shader->set_uniform("u_has_texture", &has_texture);

					// Soft particle depth blending
					int soft_blend = 0;
					if (pc.m_blend_mode == ParticleBlendMode::Soft && depth_image) {
						depth_image->bind();
						pipeline->m_shader->set_uniform_binding("u_depth_texture", depth_image->get_binding());
						float near_val = camera_comp.m_near;
						float far_val = camera_comp.m_far;
						pipeline->m_shader->set_uniform("u_near", &near_val);
						pipeline->m_shader->set_uniform("u_far", &far_val);
						soft_blend = 1;
					}
pipeline->m_shader->set_uniform("u_soft_blend", &soft_blend);

// Shadow reception
bool effective_shadow = (shadow_image != nullptr) && pc.m_receive_shadows;
if (effective_shadow) {
shadow_image->bind();
pipeline->m_shader->set_uniform_binding("u_shadow_map", shadow_image->get_binding());
}
int receive_shadows_val = effective_shadow ? 1 : 0;
pipeline->m_shader->set_uniform("u_receive_shadows", &receive_shadows_val);

// Draw instanced: shared quad VAO + per-particle instance buffer
m_quad_vao->bind();
m_quad_vao->draw_instanced(PrimitiveType::Triangles, static_cast<uint32_t>(instances.size()), pc.m_runtime.m_vbo, 2, 1);
m_quad_vao->unbind();

// Unbind resources
if (effective_shadow) {
shadow_image->unbind();
}

if (soft_blend && depth_image) {
depth_image->unbind();
}

if (pc.m_texture && pc.m_texture->m_image) {
pc.m_texture->m_image->unbind();
}

pipeline->unbind();
}
});
}

void ParticleRenderer::add_particle_shadow_passes(RenderGraph& rg, Scene* scene, std::shared_ptr<Framebuffer> const& shadow_fb, int csm_layers) {
if (!scene || !shadow_fb || !m_pipeline_shadow) return;

for (int cascade = 0; cascade < csm_layers; ++cascade) {
RenderPass::Description desc;
desc.depth_stencil_attachment.depth_load_op = LoadOp::Load;

std::string pass_name = std::string("particle-shadow-CSM") + std::to_string(cascade);
rg.add_pass(pass_name)
.set_output(shadow_fb)
.set_pass_desc(desc)
.pre_pass([shadow_fb, cascade](RenderGraphNode& node, GraphicsContext& ctx) {
shadow_fb->set_attachment_layer(0, cascade);
})
.execute([this, scene, cascade](RenderGraphNode& node, GraphicsContext& ctx) {
auto view = scene->m_registry.view<ParticleComponent, TransformComponent const>();

auto cam_entity = scene->get_main_camera();
if (!cam_entity) return;
auto& camera_comp = cam_entity->get_component<CameraComponent>();
auto view_mat = camera_comp.get_view();
glm::vec3 cam_right = glm::vec3(view_mat[0][0], view_mat[1][0], view_mat[2][0]);
glm::vec3 cam_up    = glm::vec3(view_mat[0][1], view_mat[1][1], view_mat[2][1]);

for (auto entity : view) {
auto& pc = view.get<ParticleComponent>(entity);
auto& transform = view.get<TransformComponent const>(entity);

if (pc.m_runtime.m_alive_count == 0) continue;
if (!pc.m_cast_shadows) continue;

size_t required_size = static_cast<size_t>(pc.m_max_particles) * sizeof(ParticleInstanceData);
if (!pc.m_runtime.m_vbo || pc.m_runtime.m_vbo->get_size() < required_size) {
pc.m_runtime.m_vbo = VertexBuffer::create(nullptr, required_size,
{
{DataType::Float3},
{DataType::Float},
{DataType::Float4},
{DataType::Float},
}, BufferUsage::Stream);
}

std::vector<ParticleInstanceData> instances;
instances.reserve(pc.m_runtime.m_alive_count);

for (auto const& particle : pc.m_runtime.m_particles) {
if (!particle.alive) continue;
ParticleInstanceData inst;
if (pc.m_world_space) {
inst.position = particle.position;
}
else {
inst.position = glm::vec3(transform.get_world_transform() * glm::vec4(particle.position, 1.0f));
}
inst.size = particle.size;
inst.color = particle.color;
inst.rotation = particle.rotation;
instances.push_back(inst);
}

if (instances.empty()) continue;

pc.m_runtime.m_vbo->write(instances.data(), instances.size() * sizeof(ParticleInstanceData));

m_pipeline_shadow->bind();
auto& s = m_pipeline_shadow->m_shader;

s->set_uniform_block_binding("Global", g_runtime_context.m_global->get_binding());
s->set_uniform("u_cam_right", &cam_right);
s->set_uniform("u_cam_up", &cam_up);
s->set_uniform("u_csm_index", &cascade);

int has_texture = 0;
if (pc.m_texture && pc.m_texture->m_image) {
pc.m_texture->m_image->bind();
int binding = (int)pc.m_texture->m_image->get_binding();
s->set_uniform("u_texture", &binding);
has_texture = 1;
}
s->set_uniform("u_has_texture", &has_texture);

m_quad_vao->bind();
m_quad_vao->draw_instanced(PrimitiveType::Triangles, static_cast<uint32_t>(instances.size()), pc.m_runtime.m_vbo, 2, 1);
m_quad_vao->unbind();

if (pc.m_texture && pc.m_texture->m_image) {
pc.m_texture->m_image->unbind();
}

m_pipeline_shadow->unbind();
}
});
}
}

}
