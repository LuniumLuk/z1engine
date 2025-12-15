@uniforms: {
	#include <common/uniforms.glsl>

	layout(location = 0) uniform sampler2D u_scene;
}
@stage: vert {
	#include <common/quad.glsl>
}
@stage: frag {
	layout(location = 0) out vec4 frag_color;

	layout(location = 0) in vec2 v_uv;

	// ------------------------------------------------------------
	// Utilities
	// ------------------------------------------------------------

	vec3 tonemap_reinhard(vec3 color) {
		return color / (color + vec3(1.0));
	}

	vec3 gamma_correct(vec3 color, float gamma) {
		return pow(color, vec3(1.0 / gamma));
	}

	// ------------------------------------------------------------
	// Main
	// ------------------------------------------------------------

	void main() {
		vec3 color = texture(u_scene, v_uv).rgb;

		// Exposure
		color *= u_pp_exposure;

		// Tonemapping
		color = tonemap_reinhard(color);

		// Tint
		color *= u_pp_tint.rgb;

		// Gamma correction
		color = gamma_correct(color, u_pp_gamma);

		frag_color = vec4(color, 1.0);
	}
}
