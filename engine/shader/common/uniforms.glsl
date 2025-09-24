// this is common uniforms for z1engine
// just include this file in the front of @uniforms scope
// like this:
// @uniforms: {
//   #include <common/uniforms.glsl>
///  ...
// }

#version 460 core

layout(location = 0) uniform mat4 u_projview;
layout(location = 1) uniform mat4 u_model;
layout(location = 2) uniform vec3 u_sun_direction;
layout(location = 3) uniform vec3 u_sun_intensity;
layout(location = 4) uniform vec3 u_cam_position;
