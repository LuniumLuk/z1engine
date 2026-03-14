#include "pch.h"
#include "render/renderer/particle_renderer.h"
#include "render/shader.h"
#include "render/buffer.h"
#include "render/vertex_array.h"
#include "scene/component/particle.h"
#include "scene/component/base.h"
#include "scene/scene.h"
#include "scene/entity.h"
#include "asset/asset_manager.h"

namespace z1 {

	void ParticleRenderer::init() {
		// Load particle shader
		auto particle_shader = g_runtime_context.m_asset_manager->get<Shader>("shader/particle");

		// Create pipelines for different blend modes

		// Alpha blend pipeline
		{
			Pipeline::Description desc;
			desc.blend = true;
			desc.depth_test = false;
			desc.depth_write = false;
			desc.cull_mode = CullMode::None;
			desc.src_blend_factor = BlendFactor::SrcAlpha;
			desc.dst_blend_factor = BlendFactor::OneMinusSrcAlpha;
			desc.shader = particle_shader;
			m_pipeline_alpha = Pipeline::build(desc);
		}

		// Additive blend pipeline
		{
			Pipeline::Description desc;
			desc.blend = true;
			desc.depth_test = false;
			desc.depth_write = false;
			desc.cull_mode = CullMode::None;
			desc.src_blend_factor = BlendFactor::SrcAlpha;
			desc.dst_blend_factor = BlendFactor::One;
			desc.shader = particle_shader;
			m_pipeline_additive = Pipeline::build(desc);
		}

		// Soft blend pipeline (alpha with depth fade)
		{
			Pipeline::Description desc;
			desc.blend = true;
			desc.depth_test = false;
			desc.depth_write = false;
			desc.cull_mode = CullMode::None;
			desc.src_blend_factor = BlendFactor::SrcAlpha;
			desc.dst_blend_factor = BlendFactor::OneMinusSrcAlpha;
			desc.shader = particle_shader;
			m_pipeline_soft = Pipeline::build(desc);
		}
	}

	void ParticleRenderer::shutdown() {
		m_pipeline_alpha.reset();
		m_pipeline_additive.reset();
		m_pipeline_soft.reset();
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

	void ParticleRenderer::add_particle_pass(RenderGraph& rg, Scene* scene, std::string const& input_pass) {
		if (!scene) return;

		rg.add_pass("particles")
			.set_passthrough(input_pass)
			.execute([this, scene](RenderGraphNode& node, GraphicsContext& ctx) {
				// Particle vertex layout: position, size, color, rotation, texcoord
				struct ParticleVertex {
					glm::vec3 position;
					float size;
					glm::vec4 color;
					float rotation;
					glm::vec2 texcoord;
				};

				auto default_texture = g_runtime_context.m_asset_manager->get<Texture2D>("texture/T_white");
				auto view = scene->m_registry.view<ParticleComponent, TransformComponent const>();

				for (auto entity : view) {
					auto& pc = view.get<ParticleComponent>(entity);
					auto& transform = view.get<TransformComponent>(entity);

					// Skip if no alive particles
					if (pc.m_alive_count == 0) continue;

					// Get camera
					auto cam_entity = scene->get_main_camera();
					if (!cam_entity) continue;
					auto& camera_comp = cam_entity->get_component<CameraComponent>();

					// Initialize buffers if needed
					if (!pc.m_vbo || !pc.m_vao) {
						pc.m_vbo = VertexBuffer::create(nullptr, pc.m_max_particles * 4 * sizeof(ParticleVertex),
							{
								{DataType::Float3},  // position
								{DataType::Float},   // size
								{DataType::Float4},  // color
								{DataType::Float},   // rotation
								{DataType::Float2},  // texcoord
							}, BufferUsage::Stream);

						pc.m_vao = VertexArray::create({ pc.m_vbo }, nullptr);
					}

					// Build vertex data from alive particles
					// Each particle needs 4 vertices (for quad corners)
					std::vector<ParticleVertex> vertices;
					vertices.reserve(pc.m_alive_count * 4);

					for (const auto& particle : pc.m_particles) {
						if (!particle.alive) continue;

						// Get particle world position
						glm::vec3 particle_pos;
						if (pc.m_world_space) {
							particle_pos = particle.position;
						} else {
							particle_pos = glm::vec3(transform.get_world_transform() * glm::vec4(particle.position, 1.0f));
						}

						// Create 4 vertices per particle (shader will use gl_VertexID % 4)
						for (int i = 0; i < 4; ++i) {
							ParticleVertex v;
							v.position = particle_pos;
							v.size = particle.size;
							v.color = particle.color;
							v.rotation = particle.rotation;
							v.texcoord = glm::vec2(0.0f, 0.0f);
							vertices.push_back(v);
						}
					}

					if (vertices.empty()) continue;

					// Upload vertex data to GPU
					pc.m_vbo->write(vertices.data(), vertices.size() * sizeof(ParticleVertex));

					// Select correct pipeline based on blend mode
					auto pipeline = select_pipeline(pc.m_blend_mode);
					if (!pipeline) continue;

					pipeline->bind();

					// Set shader uniforms
					auto projview = camera_comp.get_proj() * camera_comp.get_view();
					pipeline->m_shader->set_uniform("u_projview", &projview);

					// Bind texture if available
					std::shared_ptr<Texture2D> texture_to_bind = pc.m_texture ? pc.m_texture : default_texture;
					if (texture_to_bind && texture_to_bind->m_image) {
						texture_to_bind->m_image->bind();
						int binding = (int)texture_to_bind->m_image->get_binding();
						pipeline->m_shader->set_uniform("u_texture", &binding);
					}

					// Bind VAO and draw
					pc.m_vao->bind();
					pc.m_vao->draw(PrimitiveType::Points, (uint32_t)vertices.size());
					pc.m_vao->unbind();

					if (texture_to_bind && texture_to_bind->m_image) {
						texture_to_bind->m_image->unbind();
					}

					pipeline->unbind();
				}
			});
	}

}
