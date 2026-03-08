@uniforms: {
	#include <common/uniforms.glsl>
}
@stage: vert {
#define VELOCITY
	#include <common/vert.glsl>
}
@stage: frag {
	layout(location = 6) in vec4 v_curr_clip;
	layout(location = 7) in vec4 v_prev_clip;
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

		// Handle NaN values
		velocity = vec2(
			isnan(velocity.x) ? 0.0 : velocity.x,
			isnan(velocity.y) ? 0.0 : velocity.y
		);

		frag_color = vec4(velocity, 0.0, 1.0);
	}
}
