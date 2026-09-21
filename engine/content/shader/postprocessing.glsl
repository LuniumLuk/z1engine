@uniforms: {
	#include <include/uniforms.glsl>

	LOCATION(0) uniform sampler2D u_scene;
	LOCATION(1) uniform sampler2D u_bloom_texture;
}
@stage: vert {
	#include <include/quad.glsl>
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
		// Non-spatial color pipeline only: sharpening is the gated TAA sharpen pass.
		vec3 color = texture(u_scene, v_uv).rgb;

		// Bloom
		if (u_pp_bloom_enabled > 0.5) {
			vec3 bloom = texture(u_bloom_texture, v_uv).rgb;
			// Lerp bloom intensity
			color = mix(color, color + bloom, u_pp_bloom_intensity);
		}

		// Radiance cannot be negative; Reinhard's pole at -1 would map negatives to bright output.
		color = max(color, vec3(0.0));

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
