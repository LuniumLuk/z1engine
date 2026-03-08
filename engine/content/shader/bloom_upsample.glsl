@uniforms: {
	#include <common/uniforms.glsl>

	layout(location = 0) uniform sampler2D u_src_texture;
	layout(location = 1) uniform float u_filter_radius;
}
@stage: vert {
	#include <common/quad.glsl>
}
@stage: frag {
	layout(location = 0) out vec3 upsample;

	layout(location = 0) in vec2 v_uv;

	// 3x3 Tent filter
	// Based on "Next Generation Post Processing in Call of Duty: Advanced Warfare"
	vec3 upsample_tent(sampler2D tex, vec2 uv, vec2 tex_size, float radius) {
		vec4 offset = vec4(1.0, 1.0, -1.0, 0.0) * vec4(radius);
		offset.xy *= 1.0 / tex_size;
		offset.zw *= 1.0 / tex_size;

		vec3 result = vec3(0.0);

		result += texture(tex, uv - offset.xy).rgb;
		result += texture(tex, uv - offset.wy).rgb * 2.0;
		result += texture(tex, uv - offset.zy).rgb;

		result += texture(tex, uv + offset.zw).rgb * 2.0;
		result += texture(tex, uv).rgb * 4.0;
		result += texture(tex, uv + offset.xw).rgb * 2.0;

		result += texture(tex, uv + offset.zy).rgb;
		result += texture(tex, uv + offset.wy).rgb * 2.0;
		result += texture(tex, uv + offset.xy).rgb;

		return result * (1.0 / 16.0);
	}

	void main() {
		vec2 tex_size = vec2(textureSize(u_src_texture, 0));
		upsample = upsample_tent(u_src_texture, v_uv, tex_size, u_filter_radius);
	}
}
