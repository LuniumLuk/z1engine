@uniforms: {
	#include <include/uniforms.glsl>

	layout(location = 0) uniform sampler2D u_scene;
	layout(location = 1) uniform sampler2D u_bloom_texture;
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

	vec3 sharpen(sampler2D tex, vec2 uv) {
		vec3 color = texture(tex, uv).rgb;
		float amount = 0.5; // Hardcoded sharpness

		vec2 texSize = vec2(textureSize(tex, 0));
		vec2 pixelSize = 1.0 / texSize;

		vec3 up = texture(tex, uv + vec2(0, pixelSize.y)).rgb;
		vec3 down = texture(tex, uv - vec2(0, pixelSize.y)).rgb;
		vec3 left = texture(tex, uv - vec2(pixelSize.x, 0)).rgb;
		vec3 right = texture(tex, uv + vec2(pixelSize.x, 0)).rgb;

		vec3 blurred = (up + down + left + right) * 0.25;
		return color + (color - blurred) * amount;
	}

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
		vec3 color = sharpen(u_scene, v_uv);

		// Bloom
		if (u_pp_bloom_enabled > 0.5) {
			vec3 bloom = texture(u_bloom_texture, v_uv).rgb;
			// Lerp bloom intensity
			color = mix(color, color + bloom, u_pp_bloom_intensity);
		}

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
