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

	const int GTAO_SLICES = 6;
	const int GTAO_STEPS = 8;
	const float PI = 3.14159265359;

	// Interleaved gradient noise: per-pixel slice rotation (static per frame)
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

	// March one side of a slice in screen space, tracking the max horizon cosine.
	// Blends toward new samples with distance falloff, and only slightly toward
	// lower samples (thin-occluder compensation) to reduce noise and over-darkening.
	float march_horizon(vec3 P, vec2 uv, vec2 dir, float side, vec3 V, float radius_uv) {
		float max_hcos = -1.0;
		for (int k = 1; k <= GTAO_STEPS; ++k) {
			float s = float(k) / float(GTAO_STEPS);
			vec2 sample_uv = uv + dir * side * radius_uv * s;
			float sd = texture(u_depth_texture, sample_uv).r;
			if (sd >= 1.0) continue;

			vec3 S = reconstruct_view_pos(sample_uv, sd);
			vec3 delta = S - P;
			float dist = length(delta);
			if (dist < 1e-5) continue;

			float hcos = dot(delta / dist, V);
			float falloff = clamp(1.0 - (dist * dist) / (u_radius * u_radius), 0.0, 1.0);

			if (hcos >= max_hcos) {
				max_hcos = mix(max_hcos, hcos, falloff);
			}
			else {
				max_hcos = mix(max_hcos, hcos, 0.03);
			}
		}
		return max_hcos;
	}

	void main() {
		float depth = texture(u_depth_texture, v_uv).r;
		if (depth >= 1.0) {
			ao_out = 1.0;
			return;
		}

		vec3 P = reconstruct_view_pos(v_uv, depth);
		vec3 N = normalize(mat3(u_view) * texture(u_normal_texture, v_uv).xyz);
		vec3 V = normalize(-P);

		float noise = ign(gl_FragCoord.xy);

		// World-space radius converted to screen-space UV extent at this depth:
		// pixels-per-world-unit = H_full / (2 * |z| * tan(fov/2)) = H_full * proj[1][1] / (2|z|)
		// At half-res UV units (divide by H_full, AO height = H_full/2) this cancels to:
		float radius_uv = u_radius * u_proj[1][1] / (2.0 * abs(P.z));

		float ao = 0.0;
		float total_weight = 0.0;

		for (int i = 0; i < GTAO_SLICES; ++i) {
			// Slice direction in the view XY plane, rotated per pixel
			float phi = PI / float(GTAO_SLICES) * (float(i) + noise);
			vec2 dir2d = vec2(cos(phi), sin(phi));
			vec3 dirV = vec3(dir2d, 0.0);

			// Slice plane through the view vector; project the normal onto it
			vec3 axis = normalize(cross(dirV, V));
			vec3 projN = N - axis * dot(N, axis);
			float w = length(projN);
			if (w < 1e-4) continue;

			// In-slice tangent perpendicular to the view vector (signs the normal
			// angle n). Must be cross(V, axis) - cross(axis, V) would negate n and
			// break the horizon side/clamp mapping (XeGTAO convention).
			vec3 ortho_dir = normalize(cross(V, axis));
			float cosN = clamp(dot(projN, V) / w, -1.0, 1.0);
			float n = sign(dot(ortho_dir, projN)) * acos(cosN);

			// Horizon search on both sides of the slice
			float hcos_plus = march_horizon(P, v_uv, dir2d, 1.0, V, radius_uv);
			float hcos_minus = march_horizon(P, v_uv, dir2d, -1.0, V, radius_uv);

			// Convert horizon cosines to angles from the view vector, then clamp
			// to the visible hemisphere above the tangent plane (perpendicular to N)
			float h_plus = n + clamp(acos(clamp(hcos_plus, -1.0, 1.0)) - n, -PI * 0.5, PI * 0.5);
			float h_minus = n + clamp(-acos(clamp(hcos_minus, -1.0, 1.0)) - n, -PI * 0.5, PI * 0.5);

			// Cosine-weighted normalized slice integral (Jimenez 2016):
			//   sliceAO(h) = 0.25 * (cosN + 2h*sin(n) - cos(2h - n))
			float slice_ao = 0.25 * (
				(cosN + 2.0 * h_plus * sin(n) - cos(2.0 * h_plus - n)) +
				(cosN + 2.0 * h_minus * sin(n) - cos(2.0 * h_minus - n)));

			ao += slice_ao * w;
			total_weight += w;
		}

		// The accumulated slice integral is the cosine-weighted VISIBILITY (1 = fully
		// open, 0 = fully occluded). Output it directly - inverting here would make
		// open surfaces dark (the SSAO/GTAO texture convention is 1 = no occlusion).
		float vis = total_weight > 1e-5 ? ao / total_weight : 1.0;
		vis = pow(clamp(vis, 0.0, 1.0), u_power);
		vis = 1.0 - (1.0 - vis) * u_intensity;
		ao_out = vis;
	}
}
