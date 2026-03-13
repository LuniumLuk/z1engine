@uniforms: {
	#include <common/uniforms.glslh>
	#include <common/pbr_uniforms.glslh>
}
@reflections: {
	#include <common/reflections.glslh>
	#include <common/pbr_reflections.glslh>

	s_base_color                = sampler2D texture/T_white
	u_base_color_uv_set         = int 0
	u_base_color_factor         = vec4 1.0 1.0 1.0 1.0

	s_metallic_roughness        = sampler2D texture/T_white
	u_metallic_roughness_uv_set = int 0
	u_metallic_factor           = float 0.5
	u_roughness_factor          = float 0.5
}
@stage: vert {
	#include <common/vert.glslh>
}
@stage: frag {
	#include <common/frag_attrs.glslh>

	// G-buffer MRT outputs (frag_color is RT0 from frag_attrs at location 0)
	layout(location = 1) out vec4 gbuffer_normal;
	layout(location = 2) out vec4 gbuffer_albedo;
	layout(location = 3) out vec4 gbuffer_metallic_roughness;

	vec2 get_uv(int uv_set) {
		if (uv_set == 1) return v_texcoord1;
		return v_texcoord0;
	}

	void main() {
		// Alpha handling
		vec4 base_color_sample = texture(s_base_color, get_uv(u_base_color_uv_set));
		base_color_sample.rgb = pow(base_color_sample.rgb, vec3(2.2));
		vec4 base_color_vec = base_color_sample * v_color * u_base_color_factor;
		float alpha = base_color_vec.a;

		if (u_alpha_mode == 0) {
			alpha = 1.0;
		}
		else if (u_alpha_mode == 1) {
			if (alpha < u_alpha_cutoff) discard;
		}

		// Normal mapping
		vec3 normal_map = texture(s_normal, get_uv(u_normal_uv_set)).rgb * 2.0 - 1.0;
		vec3 T_vec = normalize(v_tangent.xyz);
		vec3 N_raw = normalize(v_normal);
		vec3 N;
		if (abs(v_tangent.w) > 0.0) {
			vec3 B = normalize(cross(N_raw, T_vec) * v_tangent.w);
			mat3 TBN = mat3(T_vec, B, N_raw);
			N = normalize(TBN * normal_map);
		} else {
			vec3 dp1 = dFdx(v_world_position);
			vec3 dp2 = dFdy(v_world_position);
			vec2 duv1 = dFdx(v_texcoord0);
			vec2 duv2 = dFdy(v_texcoord0);
			if (dot(duv1, duv1) + dot(duv2, duv2) < 1e-6) {
				N = N_raw;
			} else {
				vec3 T_fb = normalize(dp1 * duv2.y - dp2 * duv1.y);
				vec3 B_fb = normalize(cross(N_raw, T_fb));
				mat3 TBN = mat3(T_fb, B_fb, N_raw);
				N = normalize(TBN * normal_map);
			}
		}

		// Metallic / roughness
		vec2 rm = texture(s_metallic_roughness, get_uv(u_metallic_roughness_uv_set)).gb;
		rm = pow(rm, vec2(2.2));
		float roughness = clamp(rm.x * u_roughness_factor, 0.04, 1.0);
		float metallic  = rm.y * u_metallic_factor;

		// Write G-buffer:
		// RT0 (frag_color): world position + alpha
		frag_color = vec4(v_world_position, alpha);
		// RT1: world normal (encoded in [-1,1] range, stored as-is in RGB16F)
		gbuffer_normal = vec4(N, 0.0);
		// RT2: albedo (linear-space base color)
		gbuffer_albedo = vec4(base_color_vec.rgb, alpha);
		// RT3: metallic (R), roughness (G)
		gbuffer_metallic_roughness = vec4(metallic, roughness, 0.0, 0.0);
	}
}
