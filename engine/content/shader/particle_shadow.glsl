@uniforms: {
	#include <include/uniforms.glsl>
	uniform sampler2D u_texture;
	uniform int u_has_texture;
	uniform vec3 u_cam_right;
	uniform vec3 u_cam_up;
	uniform int u_csm_index;
}

@stage: vert {
	layout(location = 0) in vec2 a_quad_offset;
	layout(location = 1) in vec2 a_texcoord;
	layout(location = 2) in vec3 a_position;
	layout(location = 3) in float a_size;
	layout(location = 4) in vec4 a_color;
	layout(location = 5) in float a_rotation;

	layout(location = 0) out vec4 v_color;
	layout(location = 1) out vec2 v_texcoord;

	void main() {
		float angle_rad = radians(a_rotation);
		float cos_a = cos(angle_rad);
		float sin_a = sin(angle_rad);

		mat2 rot_matrix = mat2(cos_a, -sin_a, sin_a, cos_a);
		vec2 rotated_offset = rot_matrix * a_quad_offset;
		vec2 scaled_offset = rotated_offset * a_size;

		vec3 world_pos = a_position + u_cam_right * scaled_offset.x + u_cam_up * scaled_offset.y;
		gl_Position = u_sun_projview[u_csm_index] * vec4(world_pos, 1.0);

		v_color = a_color;
		v_texcoord = a_texcoord;
	}
}

@stage: frag {
	layout(location = 0) in vec4 v_color;
	layout(location = 1) in vec2 v_texcoord;

	void main() {
		float alpha = v_color.a;
		if (u_has_texture != 0) {
			alpha *= texture(u_texture, v_texcoord).a;
		}
		if (alpha < 0.1) discard;
	}
}
