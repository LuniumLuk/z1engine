// this is common uniforms for z1engine
// just include this file in the front of @uniforms scope
// like this:
// @uniforms: {
//   #include <common/uniforms.glsl>
///  ...
// }

#version 460 core

uniform mat4 u_model;

// global constant buffer
layout (std140) uniform Global {
	mat4  u_projview;
	mat4  u_prev_projview;
	mat4  u_sun_projview;
	vec4  u_sun_direction;
	vec4  u_sun_intensity;
	vec4  u_sun_ambient;
	vec4  u_cam_position;
	float u_taa_enabled;
	float u_taa_blend;
	float u_pp_exposure;
	float u_pp_gamma;
	vec4  u_pp_tint;
};

uniform sampler2D u_shadow_map;
