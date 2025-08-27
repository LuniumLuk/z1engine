// this is a common vertex shader for z1engine
// just include this file in the @stage: vert scope
// like this:
// @stage: vert {
//   #include <common/vert.glsl>
// }

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord0;
layout(location = 3) in vec2 a_texcoord1;
layout(location = 4) in vec4 a_tangent;
layout(location = 5) in vec4 a_color;

layout(location = 0) out vec3 v_world_position;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_texcoord0;
layout(location = 3) out vec2 v_texcoord1;
layout(location = 4) out vec4 v_color;

void main() {
	vec3 world_position = (u_model * vec4(a_position, 1.0)).xyz;
	gl_Position = u_projview * vec4(world_position, 1.0);
	v_world_position = world_position;
	v_normal = mat3(transpose(inverse(u_model))) * a_normal;
	v_texcoord0 = a_texcoord0;
	v_texcoord1 = a_texcoord1;
	v_color = a_color;
}
