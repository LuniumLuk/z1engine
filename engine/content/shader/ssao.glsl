@uniforms: {
	#include <include/uniforms.glsl>

	LOCATION(0) uniform sampler2D u_depth_texture;
	LOCATION(1) uniform sampler2D u_normal_texture;
	LOCATION(2) uniform mat4 u_proj;
	LOCATION(3) uniform mat4 u_inv_proj;
	LOCATION(4) uniform mat4 u_view;
	LOCATION(5) uniform float u_radius;
	LOCATION(6) uniform float u_bias;
	LOCATION(7) uniform float u_intensity;
	LOCATION(8) uniform float u_power;
}
@stage: vert {
	#include <include/quad.glsl>
}
@stage: frag {
	layout(location = 0) out float ao_out;

	layout(location = 0) in vec2 v_uv;

	const int SSAO_SAMPLES = 16;
	const float PI = 3.14159265359;

	// Interleaved gradient noise: per-pixel random rotation without a noise texture.
	// Static per frame (no time term) so the spatial noise is removed by the blur.
	float ign(vec2 p) {
		float n = sin(dot(p, vec2(12.9898, 78.233)));
		return fract(n * 43758.5453);
	}

	// Reconstruct view-space position from non-linear depth
	vec3 reconstruct_view_pos(vec2 uv, float depth) {
		vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
		vec4 view = u_inv_proj * clip;
		return view.xyz / view.w;
	}

	// Golden-angle spiral hemisphere kernel (tangent space, +z up)
	vec3 get_kernel_sample(int i) {
		float phi = float(i) * 2.399963229728653;
		float r = sqrt((float(i) + 0.5) / float(SSAO_SAMPLES));
		float x = cos(phi) * r;
		float y = sin(phi) * r;
		float z = sqrt(max(0.0, 1.0 - x * x - y * y));
		return vec3(x, y, z);
	}

	void main() {
		float depth = texture(u_depth_texture, v_uv).r;
		if (depth >= 1.0) {
			ao_out = 1.0;
			return;
		}

		vec3 view_pos = reconstruct_view_pos(v_uv, depth);
		vec3 normal = normalize(mat3(u_view) * texture(u_normal_texture, v_uv).xyz);

		// Build a per-pixel rotated tangent basis to hide kernel banding
		float ang = ign(gl_FragCoord.xy) * 2.0 * PI;
		vec3 random_vec = normalize(vec3(cos(ang), sin(ang), 1.0));
		vec3 tangent = normalize(random_vec - normal * dot(random_vec, normal));
		vec3 bitangent = cross(normal, tangent);
		mat3 TBN = mat3(tangent, bitangent, normal);

		float occlusion = 0.0;
		for (int i = 0; i < SSAO_SAMPLES; ++i) {
			vec3 sample_offset = get_kernel_sample(i);
			// Densify samples near the surface
			sample_offset *= mix(0.1, 1.0, float(i * i) / float(SSAO_SAMPLES * SSAO_SAMPLES));
			vec3 sample_pos = view_pos + TBN * sample_offset * u_radius;

			vec4 clip = u_proj * vec4(sample_pos, 1.0);
			clip.xyz /= clip.w;
			vec2 sample_uv = clip.xy * 0.5 + 0.5;
			if (sample_uv.x < 0.0 || sample_uv.x > 1.0 || sample_uv.y < 0.0 || sample_uv.y > 1.0) {
				continue;
			}

			float sample_depth = texture(u_depth_texture, sample_uv).r;
			if (sample_depth >= 1.0) continue;

			float sample_view_z = reconstruct_view_pos(sample_uv, sample_depth).z;
			float range_check = smoothstep(0.0, 1.0, u_radius / abs(view_pos.z - sample_view_z));
			occlusion += (sample_view_z >= sample_pos.z + u_bias ? 1.0 : 0.0) * range_check;
		}

		float ao = 1.0 - occlusion / float(SSAO_SAMPLES);
		ao = pow(clamp(ao, 0.0, 1.0), u_power);
		ao = 1.0 - (1.0 - ao) * u_intensity;
		ao_out = ao;
	}
}
