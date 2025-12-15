@uniforms: {
	#include <common/uniforms.glsl>

	uniform sampler2D u_current_color;
	uniform sampler2D u_history_color;
	uniform sampler2D u_velocity;
}
@stage: vert {
	#include <common/quad.glsl>
}
@stage: frag {
	layout(location = 0) in vec2 v_uv;
	layout(location = 0) out vec4 frag_color;

	void main() {
		// Read motion vector (in UV space)
		vec2 velocity = texture(u_velocity, v_uv).xy;

		// Reproject UV into previous frame
		vec2 prev_uv = v_uv - velocity;

		// Sample current and history
		vec3 current = texture(u_current_color, v_uv).rgb;
		vec3 history = texture(u_history_color, prev_uv).rgb;

		// Simple temporal blend
		vec3 color = mix(current, history, u_taa_blend);

		frag_color = vec4(color, 1.0);
	}
}