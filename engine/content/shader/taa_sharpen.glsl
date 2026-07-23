@uniforms: {
	#include <include/uniforms.glsl>

	uniform sampler2D u_src_texture;
}
@stage: vert {
	#include <include/quad.glsl>
}
@stage: frag {
	layout(location = 0) in vec2 v_uv;
	layout(location = 0) out vec4 frag_color;

	// Luma calculation (BT.709)
	float Luma(vec3 c) {
		return dot(c, vec3(0.2126, 0.7152, 0.0722));
	}

	void main() {
		if (u_taa_enabled < 0.5 || u_taa_sharpen_enabled < 0.5) {
			frag_color = vec4(texture(u_src_texture, v_uv).rgb, 1.0);
			return;
		}

		ivec2 texSize = textureSize(u_src_texture, 0);
		vec2 texelSize = 1.0 / vec2(texSize);

		// Sample center and 3x3 neighborhood for unsharp mask
		vec3 c00 = texture(u_src_texture, v_uv + vec2(-1, -1) * texelSize).rgb;
		vec3 c10 = texture(u_src_texture, v_uv + vec2( 0, -1) * texelSize).rgb;
		vec3 c20 = texture(u_src_texture, v_uv + vec2( 1, -1) * texelSize).rgb;
		vec3 c01 = texture(u_src_texture, v_uv + vec2(-1,  0) * texelSize).rgb;
		vec3 c11 = texture(u_src_texture, v_uv).rgb;
		vec3 c21 = texture(u_src_texture, v_uv + vec2( 1,  0) * texelSize).rgb;
		vec3 c02 = texture(u_src_texture, v_uv + vec2(-1,  1) * texelSize).rgb;
		vec3 c12 = texture(u_src_texture, v_uv + vec2( 0,  1) * texelSize).rgb;
		vec3 c22 = texture(u_src_texture, v_uv + vec2( 1,  1) * texelSize).rgb;

		// 3x3 box blur
		vec3 blur = (c00 + c10 + c20 + c01 + c11 + c21 + c02 + c12 + c22) / 9.0;

		// Unsharp mask: sharp = color + strength * (color - blur)
		vec3 sharp = c11 + u_taa_sharpen_strength * (c11 - blur);

		// Edge-stop: use luma gradient magnitude to avoid sharpening flat areas
		float lumaCenter = Luma(c11);
		float lumaLeft   = Luma(c01);
		float lumaRight  = Luma(c21);
		float lumaUp     = Luma(c10);
		float lumaDown   = Luma(c12);
		float edgeGradient = abs(lumaLeft - lumaRight) + abs(lumaUp - lumaDown);

		// Reduce sharpening on flat areas (low gradient)
		// Reduce sharpening on very sharp edges (prevent haloing)
		float edgeFactor = smoothstep(0.005, 0.05, edgeGradient) * (1.0 - smoothstep(0.3, 0.5, edgeGradient));
		edgeFactor = max(edgeFactor, 0.0);

		vec3 result = mix(c11, sharp, edgeFactor);

		// Prevent negative values from oversharpening
		result = max(vec3(0.0), result);

		frag_color = vec4(result, 1.0);
	}
}
