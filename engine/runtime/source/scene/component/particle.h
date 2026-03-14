#pragma once

#include "core/core.h"
#include "scene/component/base.h"
#include "asset/texture.h"
#include "render/buffer.h"
#include <glm/glm.hpp>
#include <vector>

namespace z1 {

	enum class ParticleBlendMode : uint8_t {
		Alpha,
		Additive,
		Soft
	};

	enum class EmitterShape : uint8_t {
		Point,
		Sphere,
		Box,
		Cone
	};

	struct Particle {
		glm::vec3 position;
		glm::vec3 velocity;
		glm::vec4 color;
		float size;
		float birth_size;
		float rotation;
		float rotation_speed;
		float age;
		float lifetime;
		bool alive;
	};

	struct ParticleRuntimeState {
		std::vector<Particle> m_particles;
		std::vector<uint32_t> m_free_list;
		uint32_t m_alive_count = 0;
		float m_emission_accumulator = 0.0f;
		uint32_t m_total_emitted = 0;
		std::shared_ptr<VertexBuffer> m_vbo;	// per-particle instance data
	};

	REFLECTED_STRUCT(ParticleComponent) : Requires<TransformComponent> {
		// Emitter configuration (reflected, serialized)
		uint32_t m_max_particles = 1000;
		float m_emission_rate = 50.0f;
		uint32_t m_burst_count = 0;
		glm::vec2 m_lifetime = glm::vec2(1.0f, 3.0f);
		glm::vec2 m_initial_speed = glm::vec2(1.0f, 5.0f);
		glm::vec3 m_direction = glm::vec3(0.0f, 1.0f, 0.0f);
		float m_direction_spread = 0.3f;
		glm::vec3 m_gravity = glm::vec3(0.0f, -9.81f, 0.0f);
		float m_damping = 0.0f;
		glm::vec2 m_initial_size = glm::vec2(0.1f, 0.3f);
		glm::vec2 m_size_over_life = glm::vec2(1.0f, 0.0f);
		glm::vec4 m_initial_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		glm::vec4 m_end_color = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
		std::shared_ptr<Texture2D> m_texture = nullptr;
		ParticleBlendMode m_blend_mode = ParticleBlendMode::Additive;
		EmitterShape m_emitter_shape = EmitterShape::Point;
		float m_shape_radius = 0.5f;
		glm::vec3 m_shape_extents = glm::vec3(0.5f, 0.5f, 0.5f);
		bool m_world_space = true;
		bool m_loop = true;
		bool m_playing = true;
		bool m_sort_by_depth = false;
		glm::vec2 m_initial_rotation = glm::vec2(0.0f, 360.0f);
		glm::vec2 m_rotation_speed = glm::vec2(0.0f, 0.0f);

		// Runtime state (not reflected, not serialized)
		ParticleRuntimeState m_runtime;

		ParticleComponent() = default;
		DISABLE_COPY(ParticleComponent)

		void emit_burst(uint32_t count);
	};

	// Reflected fields for editor
	REFLECTED_FIELD(ParticleComponent, m_max_particles,      FF_Default, "[input]min=1,max=100000")
	REFLECTED_FIELD(ParticleComponent, m_emission_rate,      FF_Default, "[input]min=0")
	REFLECTED_FIELD(ParticleComponent, m_burst_count,        FF_Default, "[input]min=0")
	REFLECTED_FIELD(ParticleComponent, m_lifetime,           FF_Default, "[drag]min=0")
	REFLECTED_FIELD(ParticleComponent, m_initial_speed,      FF_Default, "[drag]min=0")
	REFLECTED_FIELD(ParticleComponent, m_direction,          FF_Default, "[drag]")
	REFLECTED_FIELD(ParticleComponent, m_direction_spread,   FF_Default, "[slider]min=0,max=1")
	REFLECTED_FIELD(ParticleComponent, m_gravity,            FF_Default, "[drag]")
	REFLECTED_FIELD(ParticleComponent, m_damping,            FF_Default, "[slider]min=0,max=1")
	REFLECTED_FIELD(ParticleComponent, m_initial_size,       FF_Default, "[drag]min=0")
	REFLECTED_FIELD(ParticleComponent, m_size_over_life,     FF_Default, "[drag]min=0")
	REFLECTED_FIELD(ParticleComponent, m_initial_color,      FF_Default, "[color]")
	REFLECTED_FIELD(ParticleComponent, m_end_color,          FF_Default, "[color]")
	REFLECTED_FIELD(ParticleComponent, m_texture,            FF_Default)
	REFLECTED_FIELD(ParticleComponent, m_blend_mode,         FF_Default)
	REFLECTED_FIELD(ParticleComponent, m_emitter_shape,      FF_Default)
	REFLECTED_FIELD(ParticleComponent, m_shape_radius,       FF_Default, "[drag]min=0")
	REFLECTED_FIELD(ParticleComponent, m_shape_extents,      FF_Default, "[drag]min=0")
	REFLECTED_FIELD(ParticleComponent, m_world_space,        FF_Default)
	REFLECTED_FIELD(ParticleComponent, m_loop,               FF_Default)
	REFLECTED_FIELD(ParticleComponent, m_playing,            FF_Default)
	REFLECTED_FIELD(ParticleComponent, m_sort_by_depth,      FF_Default)
	REFLECTED_FIELD(ParticleComponent, m_initial_rotation,   FF_Default, "[drag]")
	REFLECTED_FIELD(ParticleComponent, m_rotation_speed,     FF_Default, "[drag]")

	// Reflect enums
	REFLECT_ENUM(ParticleBlendMode, Alpha)
	REFLECT_ENUM(ParticleBlendMode, Additive)
	REFLECT_ENUM(ParticleBlendMode, Soft)

	REFLECT_ENUM(EmitterShape, Point)
	REFLECT_ENUM(EmitterShape, Sphere)
	REFLECT_ENUM(EmitterShape, Box)
	REFLECT_ENUM(EmitterShape, Cone)

}
