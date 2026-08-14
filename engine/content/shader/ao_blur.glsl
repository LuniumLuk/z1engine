@uniforms: {
	#include <include/uniforms.glsl>

	LOCATION(1) uniform sampler2D u_depth_texture;
	LOCATION(2) uniform mat4 u_proj;
	LOCATION(3) uniform vec2 u_texel_size;
	LOCATION(4) uniform float u_strength;
}
@stage: vert {
	#include <include/quad.glsl>
}
@stage: frag {
	layout(location = 0) out float ao_out;

	layout(location = 0) in vec2 v_uv;

	// Reconstruct view-space depth (z) from non-linear depth:
	// view.z = proj[2][3] / (-z_ndc - proj[2][2])
	float view_z(vec2 uv) {
		float d = texture(u_depth_texture, uv).r;
		if (d >= 1.0) return 100000.0;
		float z_ndc = d * 2.0 - 1.0;
		return u_proj[2][3] / (-z_ndc - u_proj[2][2]);
	}

	void main() {
		float center_ao = texture(u_ao_texture, v_uv).r;
		float center_z = view_z(v_uv);

		// 5x5 depth-edge-preserving Gaussian blur
		float total = 0.0;
		float weight_sum = 0.0;
		for (int x = -2; x <= 2; ++x) {
			for (int y = -2; y <= 2; ++y) {
				vec2 off = vec2(float(x), float(y)) * u_texel_size;

				float z = view_z(v_uv + off);
				// Scale-invariant depth edge weight (relative depth difference)
				float dz = abs(z - center_z);
				float edge = smoothstep(0.02, 0.1, dz / max(abs(center_z), 1e-4));

				float gauss = exp(-float(x * x + y * y) / (2.0 * u_strength * u_strength + 1e-5));

				float weight = gauss * (1.0 - edge);
				total += texture(u_ao_texture, v_uv + off).r * weight;
				weight_sum += weight;
			}
		}

		ao_out = weight_sum > 1e-6 ? total / weight_sum : center_ao;
	}
}
