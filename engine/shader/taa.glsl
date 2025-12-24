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
		vec4 velocity = texture(u_velocity, v_uv);

		// Reproject UV into previous frame
		vec2 prev_uv = v_uv - velocity.xy;

		// Sample current and history
		vec3 current = texture(u_current_color, v_uv).rgb;

		if (u_taa_enabled < 0.5) {
			frag_color = vec4(current, 1.0);
			return;
		}

		vec3 history = texture(u_history_color, prev_uv).rgb;

		// Adjust blend factor based on motion alpha
		// With velocity.a = 0.0 (static, no draw), alpha = 0.0 (full current)
		float alpha = mix(0.0, u_taa_blend, velocity.a);

		// Simple temporal blend
		vec3 color = mix(current, history, alpha);

		frag_color = vec4(color, 1.0);
	}
}