@uniforms: {
	#include <common/uniforms.glsl>
}
@stage: vert {
	layout(location = 0) in vec3 a_position;

	layout(location = 0) out vec4 v_curr_clip;
	layout(location = 1) out vec4 v_prev_clip;

	void main() {
		vec4 world_position = u_model * vec4(a_position, 1.0);

		v_curr_clip = u_projview * world_position;
		v_prev_clip = u_prev_projview * world_position;

		gl_Position = v_curr_clip;
	}
}
@stage: frag {
	layout(location = 0) in vec4 v_curr_clip;
	layout(location = 1) in vec4 v_prev_clip;
	layout(location = 0) out vec4 frag_color;

	void main() {
		// Perspective divide
		vec2 curr_ndc = v_curr_clip.xy / v_curr_clip.w;
		vec2 prev_ndc = v_prev_clip.xy / v_prev_clip.w;

		// Convert NDC (-1..1) to UV (0..1)
		vec2 curr_uv = curr_ndc * 0.5 + 0.5;
		vec2 prev_uv = prev_ndc * 0.5 + 0.5;

		// Velocity in UV space
		vec2 velocity = curr_uv - prev_uv;
		frag_color = vec4(velocity, 0.0, 1.0);
	}
}