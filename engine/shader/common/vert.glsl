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
layout(location = 6) in vec4 a_joints;
layout(location = 7) in vec4 a_weights;

layout(location = 0) out vec3 v_world_position;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_texcoord0;
layout(location = 3) out vec2 v_texcoord1;
layout(location = 4) out vec4 v_tangent;
layout(location = 5) out vec4 v_color;

#ifdef VELOCITY
layout(location = 6) out vec4 v_curr_clip;
layout(location = 7) out vec4 v_prev_clip;
#endif

uniform mat4 u_bone_matrices[100];
uniform int u_has_skinning;

void main() {
	mat4 skin_matrix = mat4(1.0);
	if (u_has_skinning > 0) {
		skin_matrix =
			u_bone_matrices[int(a_joints.x)] * a_weights.x +
			u_bone_matrices[int(a_joints.y)] * a_weights.y +
			u_bone_matrices[int(a_joints.z)] * a_weights.z +
			u_bone_matrices[int(a_joints.w)] * a_weights.w;
	}

	vec4 local_position = vec4(a_position, 1.0);
	vec3 local_normal = a_normal;

	if (u_has_skinning > 0) {
		local_position = skin_matrix * local_position;
		local_normal = mat3(skin_matrix) * local_normal;
	}

	vec3 world_position = (u_model * local_position).xyz;
#ifdef SHADOW
	gl_Position = u_sun_projview[u_csm_index] * vec4(world_position, 1.0);
#else
	gl_Position = u_projview * vec4(world_position, 1.0);
#endif
	v_world_position = world_position;
	v_normal = mat3(transpose(inverse(u_model))) * local_normal;
	v_texcoord0 = a_texcoord0;
	v_texcoord0.y = 1.0 - v_texcoord0.y; // flip y for opengl
	v_texcoord1 = a_texcoord1;
	v_texcoord1.y = 1.0 - v_texcoord1.y; // flip y for opengl
	v_tangent = a_tangent;
	v_color = a_color;

#ifdef VELOCITY
	v_curr_clip = u_projview * vec4(world_position, 1.0);
	v_prev_clip = u_prev_projview * vec4(world_position, 1.0);
#endif

}
