@uniforms: {
	#include <include/uniforms.glsl>

	LOCATION(0) uniform sampler2D u_src_texture;
	LOCATION(1) uniform vec2 u_src_resolution;
	LOCATION(2) uniform int u_mip_level;
}
@stage: vert {
	#include <include/quad.glsl>
}
@stage: frag {
	layout(location = 0) out vec3 upsample;

	layout(location = 0) in vec2 v_uv;

	vec3 pow_vec3(vec3 v, float p) {
		return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
	}

	const float s_gamma = 2.2;
	const float s_inv_gamma = 1.0 / s_gamma;

	// sRGB => Linear
	// Digital ITU BT.709 :  Exact acquisition and delivery for HDTV programs
	vec3 to_linear(vec3 sRGB) {
		return sRGB * (sRGB * (sRGB * 0.305306011 + 0.682171111) + 0.012522878);
	}

	// 13-tap downsampling filter (linear sampling assumed)
	// Based on "Next Generation Post Processing in Call of Duty: Advanced Warfare"
	vec3 downsample_13tap(sampler2D tex, vec2 uv, vec2 tex_size) {
		vec2 pixel_size = 1.0 / tex_size;

		vec3 a = texture(tex, uv + vec2(-2,  2) * pixel_size).rgb;
		vec3 b = texture(tex, uv + vec2( 0,  2) * pixel_size).rgb;
		vec3 c = texture(tex, uv + vec2( 2,  2) * pixel_size).rgb;
		vec3 d = texture(tex, uv + vec2(-2,  0) * pixel_size).rgb;
		vec3 e = texture(tex, uv + vec2( 0,  0) * pixel_size).rgb;
		vec3 f = texture(tex, uv + vec2( 2,  0) * pixel_size).rgb;
		vec3 g = texture(tex, uv + vec2(-2, -2) * pixel_size).rgb;
		vec3 h = texture(tex, uv + vec2( 0, -2) * pixel_size).rgb;
		vec3 i = texture(tex, uv + vec2( 2, -2) * pixel_size).rgb;

		vec3 j = texture(tex, uv + vec2(-1,  1) * pixel_size).rgb;
		vec3 k = texture(tex, uv + vec2( 1,  1) * pixel_size).rgb;
		vec3 l = texture(tex, uv + vec2(-1, -1) * pixel_size).rgb;
		vec3 m = texture(tex, uv + vec2( 1, -1) * pixel_size).rgb;

		vec3 result = e * 0.125;
		result += (a + c + g + i) * 0.03125;
		result += (b + d + f + h) * 0.0625;
		result += (j + k + l + m) * 0.125;
		return result;
	}

	// Quadratic threshold curve
	// curve = (threshold - knee, knee * 2, 0.25 / knee)
	vec4 quadratic_threshold(vec4 color, float threshold, vec3 curve) {
		// Pixel brightness
		float br = max(color.r, max(color.g, color.b));

		// Under-threshold part: quadratic curve
		float rq = clamp(br - curve.x, 0.0, curve.y);
		rq = curve.z * rq * rq;

		// Combine and apply the brightness response curve.
		color.rgb *= max(rq, br - threshold) / max(br, 1e-4);

		return color;
	}

	void main() {
		vec3 color = downsample_13tap(u_src_texture, v_uv, vec2(textureSize(u_src_texture, 0)));
		vec3 upsample_out = color;

		if (u_mip_level == 0) {
			// Prefilter step
			// We can do thresholding here
			float knee = u_pp_bloom_knee;
			vec3 curve = vec3(u_pp_bloom_threshold - knee, knee * 2.0, 0.25 / (knee + 0.00001));

			vec4 c = quadratic_threshold(vec4(color, 1.0), u_pp_bloom_threshold, curve);
			upsample_out = c.rgb;
		}

		upsample = upsample_out;
	}
}
