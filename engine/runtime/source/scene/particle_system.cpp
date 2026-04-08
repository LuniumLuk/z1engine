#include "pch.h"
#include "scene/particle_system.h"
#include "scene/component/particle.h"
#include "scene/component/base.h"
#include "scene/entity.h"
#include "render/global.h"
#include "util/random_utils.h"
#include <algorithm>
#include <glm/gtc/constants.hpp>

namespace z1 {

	// Helper: Generate random direction in a cone
	static glm::vec3 random_direction_in_cone(glm::vec3 const& base_direction, float spread) {
		// spread: 0 = focused beam, 1 = full hemisphere
		// spread is a cone half-angle as fraction of pi/2 radians (90 degrees)

		// Normalize base direction (fallback to up if zero-length)
		float len = glm::length(base_direction);
		glm::vec3 dir = len > 1e-6f ? base_direction / len : glm::vec3(0.0f, 1.0f, 0.0f);

		// Generate random point on unit sphere
		float theta = Random::rfloat(0.0f, glm::two_pi<float>());
		float phi = acos(1.0f - spread * Random::rfloat(0.0f, 1.0f)); // spread controls the cone

		glm::vec3 random_offset(
			sin(phi) * cos(theta),
			sin(phi) * sin(theta),
			cos(phi)
		);

		// Find perpendicular axes
		glm::vec3 up = abs(dir.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 right = glm::normalize(glm::cross(dir, up));
		up = glm::normalize(glm::cross(right, dir));

		// Rotate random offset to align with base direction
		glm::vec3 result = random_offset.x * right + random_offset.y * up + random_offset.z * dir;
		return glm::normalize(result);
	}

	// Helper: Sample emitter shape
	static glm::vec3 sample_emitter_shape(EmitterShape shape, float radius, glm::vec3 const& extents) {
		switch (shape) {
		case EmitterShape::Point:
			return glm::vec3(0.0f);

		case EmitterShape::Sphere: {
			// Random point on sphere surface
			float theta = Random::rfloat(0.0f, glm::two_pi<float>());
			float phi = acos(Random::rfloat(-1.0f, 1.0f));
			float r = radius;
			return glm::vec3(
				r * sin(phi) * cos(theta),
				r * sin(phi) * sin(theta),
				r * cos(phi)
			);
		}

		case EmitterShape::Box: {
			// Random point in box
			return glm::vec3(
				Random::rfloat(-extents.x, extents.x),
				Random::rfloat(-extents.y, extents.y),
				Random::rfloat(-extents.z, extents.z)
			);
		}

		case EmitterShape::Cone: {
			// Random point on cone base disk
			float r = radius * sqrt(Random::rfloat(0.0f, 1.0f));
			float theta = Random::rfloat(0.0f, glm::two_pi<float>());
			return glm::vec3(
				r * cos(theta),
				r * sin(theta),
				0.0f
			);
		}

		default:
			return glm::vec3(0.0f);
		}
	}

	// Helper: Spawn a single particle
	static void spawn_particle(
		ParticleComponent& pc,
		uint32_t index,
		glm::vec3 const& emitter_position,
		glm::mat4 const& emitter_rotation
	) {
		auto& p = pc.m_runtime.m_particles[index];

		// Sample emitter shape
		glm::vec3 local_pos = sample_emitter_shape(pc.m_emitter_shape, pc.m_shape_radius, pc.m_shape_extents);

		if (pc.m_world_space) {
			p.position = emitter_position + glm::vec3(emitter_rotation * glm::vec4(local_pos, 0.0f));
		}
		else {
			p.position = local_pos;
		}

		// Direction with spread
		glm::vec3 direction = random_direction_in_cone(pc.m_direction, pc.m_direction_spread);
		float speed = Random::rfloat(pc.m_initial_speed.x, pc.m_initial_speed.y);

		p.velocity = direction * speed;
		if (pc.m_world_space) {
			// Rotate velocity to match emitter orientation
			p.velocity = glm::vec3(emitter_rotation * glm::vec4(p.velocity, 0.0f));
		}

		// Lifetime
		p.lifetime = Random::rfloat(pc.m_lifetime.x, pc.m_lifetime.y);
		p.age = 0.0f;

		// Color
		p.color = pc.m_initial_color;

		// Size
		p.birth_size = Random::rfloat(pc.m_initial_size.x, pc.m_initial_size.y);
		p.size = p.birth_size;

		// Rotation
		p.rotation = Random::rfloat(pc.m_initial_rotation.x, pc.m_initial_rotation.y);
		p.rotation_speed = Random::rfloat(pc.m_rotation_speed.x, pc.m_rotation_speed.y);

		// Mark as alive
		p.alive = true;
	}

	// Helper: Spawn a burst of particles
	static void emit_burst_internal(
		ParticleComponent& pc,
		uint32_t count,
		glm::vec3 const& emitter_position,
		glm::mat4 const& emitter_rotation
	) {
		for (uint32_t i = 0; i < count && !pc.m_runtime.m_free_list.empty(); ++i) {
			uint32_t slot = pc.m_runtime.m_free_list.back();
			pc.m_runtime.m_free_list.pop_back();
			spawn_particle(pc, slot, emitter_position, emitter_rotation);
			++pc.m_runtime.m_alive_count;
			++pc.m_runtime.m_total_emitted;
		}
	}

	void ParticleComponent::emit_burst(uint32_t count) {
		// Public API: spawns at origin with identity rotation.
		// When called from the update loop, emit_burst_internal is used instead
		// with the proper emitter transform.
		if (m_runtime.m_particles.empty()) return;
		emit_burst_internal(*this, count, glm::vec3(0.0f), glm::mat4(1.0f));
	}

	void ParticleSystem::update(Scene* scene, float dt) {
		PROFILE_FUNCTION();

		auto view = scene->m_registry.view<ParticleComponent, TransformComponent>();

		for (auto entity : view) {
			auto& pc = view.get<ParticleComponent>(entity);
			auto& tc = view.get<TransformComponent>(entity);

			if (!pc.m_playing) continue;

			// Clamp dt to prevent emission spikes after stalls/breakpoints
			float clamped_dt = glm::min(dt, 0.1f);

			// Get emitter transform
			glm::vec3 emitter_pos = tc.m_location;
			glm::mat4 emitter_rot = tc.get_local_rotation();

			// Initialize particle pool if needed
			if (pc.m_runtime.m_particles.empty()) {
				pc.m_runtime.m_particles.resize(pc.m_max_particles);
				pc.m_runtime.m_free_list.clear();
				pc.m_runtime.m_free_list.reserve(pc.m_max_particles);
				for (uint32_t i = pc.m_max_particles; i > 0; --i) {
					pc.m_runtime.m_particles[i - 1].alive = false;
					pc.m_runtime.m_free_list.push_back(i - 1);
				}

				// Initial burst on first pool creation
				if (pc.m_burst_count > 0) {
					emit_burst_internal(pc, pc.m_burst_count, emitter_pos, emitter_rot);
				}
			}

			// Resize if max_particles changed
			if (pc.m_runtime.m_particles.size() != pc.m_max_particles) {
				pc.m_runtime.m_particles.resize(pc.m_max_particles);
				pc.m_runtime.m_free_list.clear();
				pc.m_runtime.m_free_list.reserve(pc.m_max_particles);
				pc.m_runtime.m_alive_count = 0;
				for (uint32_t i = pc.m_max_particles; i > 0; --i) {
					auto& p = pc.m_runtime.m_particles[i - 1];
					if (!p.alive) {
						pc.m_runtime.m_free_list.push_back(i - 1);
					} else {
						++pc.m_runtime.m_alive_count;
					}
				}
				// Reset VBO so the renderer recreates it with correct capacity
				pc.m_runtime.m_vbo.reset();
			}

			// Spawn new particles (continuous emission)
			bool can_emit = pc.m_loop || pc.m_runtime.m_total_emitted < pc.m_max_particles;
			if (pc.m_emission_rate > 0.0f && !pc.m_runtime.m_free_list.empty() && can_emit) {
				pc.m_runtime.m_emission_accumulator += pc.m_emission_rate * clamped_dt;
				uint32_t particles_to_spawn = static_cast<uint32_t>(pc.m_runtime.m_emission_accumulator);
				pc.m_runtime.m_emission_accumulator -= particles_to_spawn;

				// Clamp to remaining budget for non-looping emitters
				if (!pc.m_loop) {
					uint32_t remaining = pc.m_max_particles - pc.m_runtime.m_total_emitted;
					particles_to_spawn = glm::min(particles_to_spawn, remaining);
				}

				for (uint32_t i = 0; i < particles_to_spawn && !pc.m_runtime.m_free_list.empty(); ++i) {
					uint32_t slot = pc.m_runtime.m_free_list.back();
					pc.m_runtime.m_free_list.pop_back();
					spawn_particle(pc, slot, emitter_pos, emitter_rot);
					++pc.m_runtime.m_alive_count;
					++pc.m_runtime.m_total_emitted;
				}
			}

			// Simulate alive particles
			for (uint32_t pi = 0; pi < pc.m_runtime.m_particles.size(); ++pi) {
				auto& p = pc.m_runtime.m_particles[pi];
				if (!p.alive) continue;

				p.age += clamped_dt;

				// Check lifetime
				if (p.age >= p.lifetime) {
					p.alive = false;
					--pc.m_runtime.m_alive_count;
					pc.m_runtime.m_free_list.push_back(pi);
					continue;
				}

				// Apply gravity
				p.velocity += pc.m_gravity * clamped_dt;

				// Apply damping (frame-rate independent exponential decay)
				if (pc.m_damping > 0.0f) {
					float damping_factor = powf(1.0f - pc.m_damping, clamped_dt);
					p.velocity *= damping_factor;
				}

				// Update position
				p.position += p.velocity * clamped_dt;

				// Interpolate color
				float t = p.age / p.lifetime;
				p.color = glm::mix(pc.m_initial_color, pc.m_end_color, t);

				// Interpolate size using stored birth size
				p.size = p.birth_size * glm::mix(pc.m_size_over_life.x, pc.m_size_over_life.y, t);

				// Update rotation
				p.rotation += p.rotation_speed * clamped_dt;
			}

			// Auto-stop non-looping emitters when all particles are dead
			if (!pc.m_loop && pc.m_runtime.m_total_emitted >= pc.m_max_particles && pc.m_runtime.m_alive_count == 0) {
				pc.m_playing = false;
			}

			// Compact alive particles to front (optional, for depth sorting)
			if (pc.m_sort_by_depth) {
				auto camera_entity = scene->get_main_camera();
				if (camera_entity) {
					auto& camera_comp = camera_entity->get_component<CameraComponent>();
					glm::vec3 camera_pos = camera_comp.get_position();
					std::sort(pc.m_runtime.m_particles.begin(), pc.m_runtime.m_particles.end(),
						[&](Particle const& a, Particle const& b) {
							if (!a.alive) return false;
							if (!b.alive) return true;
							float dist_a = glm::distance(a.position, camera_pos);
							float dist_b = glm::distance(b.position, camera_pos);
							return dist_a > dist_b; // back-to-front
						});

					// Rebuild free-list after sort (indices changed)
					pc.m_runtime.m_free_list.clear();
					for (uint32_t i = static_cast<uint32_t>(pc.m_runtime.m_particles.size()); i > 0; --i) {
						if (!pc.m_runtime.m_particles[i - 1].alive) {
							pc.m_runtime.m_free_list.push_back(i - 1);
						}
					}
				}
			}
		}
	}

}
