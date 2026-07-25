@uniforms: {
	LOCATION(0) uniform mat4 u_projview;
	LOCATION(1) uniform sampler2D u_texture[32];
}
@stage: vert {
	layout(location = 0) in vec3 a_position;
	layout(location = 1) in vec2 a_texcoord;
	layout(location = 2) in vec4 a_color;
	layout(location = 3) in float a_texture_id;
	layout(location = 4) in vec4 a_tiling_factor;

	layout(location = 0) out vec2 v_uv;
	layout(location = 1) out vec4 v_color;
	layout(location = 2) out float v_texture_id;

	void main() {
		gl_Position = u_projview * vec4(a_position, 1.0);
		v_uv = a_texcoord * a_tiling_factor.xy + a_tiling_factor.zw;
		v_color = a_color;
		v_texture_id = a_texture_id;
	}
}
@stage: frag {
	layout(location = 0) in vec2 v_uv;
	layout(location = 1) in vec4 v_color;
	layout(location = 2) in float v_texture_id;

	layout(location = 0) out vec4 frag_color;

	void main() {
		frag_color = v_color * texture(u_texture[int(round(v_texture_id))], v_uv);
	}
}
