// this is common uniforms for z1engine
// just include this file in the front of @uniforms scope
// like this:
// @uniforms: {
//   #include <include/uniforms.glsl>
///  ...
// }

uniform mat4 u_model;

// global constant buffer
layout (std140) uniform Global {
	mat4  u_projview;
	mat4  u_prev_projview;
	mat4  u_sun_projview[4];
	vec4  u_csm_splits;
	vec4  u_sun_direction;
	vec4  u_sun_intensity;
	vec4  u_sun_ambient;
	vec4  u_cam_position;
	float u_taa_enabled;
	float u_taa_blend;
	float u_pp_exposure;
	float u_pp_gamma;
	vec4  u_pp_tint;
	float u_pp_bloom_enabled;
	float u_pp_bloom_threshold;
	float u_pp_bloom_intensity;
	float u_pp_bloom_knee;
	// TAA upgrade
	float u_taa_variance_scale;
	float u_taa_clip_gamma;
	float u_taa_jitter_u;
	float u_taa_jitter_v;
	float u_taa_sharpen_enabled;
	float u_taa_sharpen_strength;
	// Ambient Occlusion
	float u_ao_enabled;
	float u_ao_type;
	float u_ao_radius;
	float u_ao_intensity;
	float u_ao_power;
	float u_ao_bias;
	float u_ao_blur_enabled;
	float u_ao_blur_strength;
	// Screen-space reflection (deferred-only)
	float u_ssr_enabled;
	float u_ssr_intensity;
	float u_ssr_max_distance;
	float u_ssr_thickness;
	float u_ssr_stride;
	float u_ssr_max_steps;
	float u_ssr_jitter_strength;
};

uniform sampler2DArray u_shadow_map;

// Screen-space AO texture (bound by the renderer when AO is enabled)
uniform sampler2D u_ao_texture;

struct Light {
	vec4 position;  // w = type (0:dir, 1:point, 2:spot)
	vec4 direction; // w = range
	vec4 color;     // w = intensity
	vec4 cone;      // x = inner, y = outer, z = cast_shadow
};

layout(std140) uniform Lights {
	vec4 u_lights_count;
	Light u_lights[16];
};

uniform float u_alpha_cutoff;
uniform int u_alpha_mode; // 0: opaque, 1: mask, 2: blend
