@uniforms: {
	#include <include/uniforms.glsl>

	uniform sampler2D u_scene_color;
	uniform sampler2D u_gbuffer_position;
	uniform sampler2D u_gbuffer_normal;
	uniform sampler2D u_gbuffer_albedo;
	uniform sampler2D u_gbuffer_metallic_roughness;
	uniform sampler2D u_gbuffer_depth;
}
@reflections: {
	u_scene_color               [invisible]
	u_gbuffer_position          [invisible]
	u_gbuffer_normal            [invisible]
	u_gbuffer_albedo            [invisible]
	u_gbuffer_metallic_roughness [invisible]
	u_gbuffer_depth             [invisible]
}
@stage: vert {
	#include <include/quad.glsl>
}
@stage: frag {
	layout(location = 0) out vec4 frag_color;
	layout(location = 0) in vec2 v_uv;

	float saturate(float x) {
		return clamp(x, 0.0, 1.0);
	}

	float hash12(vec2 p) {
		vec3 p3 = fract(vec3(p.xyx) * 0.1031);
		p3 += dot(p3, p3.yzx + 33.33);
		return fract((p3.x + p3.y) * p3.z);
	}

	vec3 sample_reflection_color(vec2 uv, vec3 N, float roughness, vec2 ray_dir_uv) {
		vec2 texel = vec2(dFdx(uv.x), dFdy(uv.y));
		texel = vec2(abs(texel.x), abs(texel.y));
		texel = max(texel, vec2(0.00075));

		float radius = mix(0.5, 4.0, roughness);
		vec2 dir = normalize(ray_dir_uv + vec2(1e-5, 0.0));
		vec2 ortho = vec2(-dir.y, dir.x);
		vec2 offset0 = dir * texel * radius;
		vec2 offset1 = ortho * texel * radius;

		vec3 c0 = texture(u_scene_color, uv).rgb;
		vec3 c1 = texture(u_scene_color, uv + offset0).rgb;
		vec3 c2 = texture(u_scene_color, uv - offset0).rgb;
		vec3 c3 = texture(u_scene_color, uv + offset1).rgb;
		vec3 c4 = texture(u_scene_color, uv - offset1).rgb;

		float w_center = mix(1.5, 0.8, roughness);
		float w_side = mix(0.6, 1.0, roughness);
		float w = w_center + 4.0 * w_side;
		return (c0 * w_center + (c1 + c2 + c3 + c4) * w_side) / w;
	}

	void main() {
		vec4 scene_sample = texture(u_scene_color, v_uv);
		vec4 position_alpha = texture(u_gbuffer_position, v_uv);
		if (position_alpha.w <= 0.0 || u_ssr_enabled < 0.5) {
			frag_color = scene_sample;
			return;
		}

		vec3 world_pos = position_alpha.xyz;
		vec3 N = normalize(texture(u_gbuffer_normal, v_uv).xyz);
		vec3 base_color = texture(u_gbuffer_albedo, v_uv).rgb;
		vec2 mr = texture(u_gbuffer_metallic_roughness, v_uv).rg;
		float metallic = saturate(mr.x);
		float roughness = saturate(mr.y);

		vec3 V = normalize(u_cam_position.xyz - world_pos);
		vec3 I = -V;
		vec3 R = normalize(reflect(I, N));

		float ndotv = saturate(dot(N, V));
		float fresnel = pow(1.0 - ndotv, 5.0);
		float reflectivity = mix(0.04, 1.0, metallic) * (0.2 + 0.8 * fresnel);
		reflectivity *= (1.0 - roughness);

		if (reflectivity <= 0.0001) {
			frag_color = scene_sample;
			return;
		}

		float max_steps = max(1.0, u_ssr_max_steps);
		float stride = max(0.01, u_ssr_stride);
		float max_distance = max(0.1, u_ssr_max_distance);
		float thickness = max(0.01, u_ssr_thickness);

		vec3 hit_color = vec3(0.0);
		float hit_weight = 0.0;
		vec2 hit_uv = vec2(0.0);

		vec3 ray_origin = world_pos + N * thickness;
		vec2 pixel_uv = floor(v_uv * vec2(textureSize(u_scene_color, 0)));
		float stable_jitter = hash12(pixel_uv * vec2(0.61803, 1.41421));
		float temporal_jitter = hash12(pixel_uv * vec2(0.75488, 0.56984) + vec2(u_taa_jitter_u * 4096.0, u_taa_jitter_v * 4096.0));
		float jitter = mix(stable_jitter, temporal_jitter, clamp(u_ssr_jitter_strength, 0.0, 1.0));
		float t = stride * jitter;
		float prev_t = 0.0;

		for (float i = 0.0; i < max_steps; i += 1.0) {
			prev_t = t;
			t += stride;
			vec3 sample_pos = ray_origin + R * t;

			if (distance(sample_pos, world_pos) > max_distance) {
				break;
			}

			vec4 clip = u_projview * vec4(sample_pos, 1.0);
			if (clip.w <= 0.0001) {
				break;
			}

			vec3 ndc = clip.xyz / clip.w;
			vec2 sample_uv = ndc.xy * 0.5 + 0.5;
			if (sample_uv.x <= 0.001 || sample_uv.x >= 0.999 || sample_uv.y <= 0.001 || sample_uv.y >= 0.999) {
				break;
			}

			vec4 candidate = texture(u_gbuffer_position, sample_uv);
			if (candidate.w <= 0.0) {
				continue;
			}

			float sample_depth = texture(u_gbuffer_depth, sample_uv).r;
			if (sample_depth >= 0.99999) {
				continue;
			}

			vec3 candidate_pos = candidate.xyz;
			float miss = length(candidate_pos - sample_pos);
			float adaptive_thickness = thickness * (1.0 + t * 0.05);
			if (miss <= adaptive_thickness) {
				// Refine the intersection in [prev_t, t] to reduce stair-stepping.
				float lo = prev_t;
				float hi = t;
				vec2 refined_uv = sample_uv;
				for (int refine = 0; refine < 5; ++refine) {
					float mid = 0.5 * (lo + hi);
					vec3 mid_pos = ray_origin + R * mid;
					vec4 mid_clip = u_projview * vec4(mid_pos, 1.0);
					if (mid_clip.w <= 0.0001) {
						break;
					}

					vec2 mid_uv = (mid_clip.xy / mid_clip.w) * 0.5 + 0.5;
					if (mid_uv.x <= 0.001 || mid_uv.x >= 0.999 || mid_uv.y <= 0.001 || mid_uv.y >= 0.999) {
						break;
					}

					vec4 mid_candidate = texture(u_gbuffer_position, mid_uv);
					if (mid_candidate.w <= 0.0) {
						lo = mid;
						continue;
					}

					float mid_miss = length(mid_candidate.xyz - mid_pos);
					if (mid_miss <= adaptive_thickness) {
						hi = mid;
						refined_uv = mid_uv;
					}
					else {
						lo = mid;
					}
				}

				hit_uv = refined_uv;

				vec3 next_pos = ray_origin + R * (t + stride);
				vec4 next_clip = u_projview * vec4(next_pos, 1.0);
				vec2 next_uv = hit_uv;
				if (next_clip.w > 0.0001) {
					next_uv = (next_clip.xy / next_clip.w) * 0.5 + 0.5;
				}
				vec2 ray_dir_uv = next_uv - hit_uv;
				hit_color = sample_reflection_color(hit_uv, N, roughness, ray_dir_uv);

				float edge_x = min(hit_uv.x, 1.0 - hit_uv.x) * 2.0;
				float edge_y = min(hit_uv.y, 1.0 - hit_uv.y) * 2.0;
				float edge_fade = saturate(min(edge_x, edge_y));
				float dist_fade = saturate(1.0 - t / max_distance);
				float stability = saturate(1.0 - miss / (adaptive_thickness + 1e-5));
				hit_weight = edge_fade * dist_fade * stability;
				break;
			}

		}

		vec3 reflection_tint = mix(vec3(1.0), base_color, metallic);
		float roughness_fade = 1.0 - smoothstep(0.65, 1.0, roughness);
		vec3 reflection = hit_color * reflection_tint * hit_weight * reflectivity * roughness_fade * u_ssr_intensity;
		vec3 result = scene_sample.rgb + reflection;
		frag_color = vec4(result, scene_sample.a);

		frag_color = vec4(
			isnan(frag_color.x) ? 0.0 : frag_color.x,
			isnan(frag_color.y) ? 0.0 : frag_color.y,
			isnan(frag_color.z) ? 0.0 : frag_color.z,
			isnan(frag_color.w) ? 0.0 : frag_color.w
		);
	}
}
