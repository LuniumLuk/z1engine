@uniforms: {
	#include <common/uniforms.glsl>

	uniform sampler2D s_base_color;
	uniform sampler2D s_metallic_roughness;
	uniform sampler2D s_normal;
	uniform sampler2D s_emissive;
	uniform sampler2D s_occlusion;

	uniform vec4 u_base_color_factor;
	uniform float u_roughness_factor;
	uniform float u_metallic_factor;
}
@reflections: {
	#include <common/reflections.glsl>

	s_base_color         = sampler2D texture/T_white
	s_metallic_roughness = sampler2D texture/T_white
	s_normal             = sampler2D texture/T_normal
	s_emissive           = sampler2D texture/T_black
	s_occlusion          = sampler2D texture/T_white

	u_base_color_factor  = vec4 1.0 1.0 1.0 1.0
	u_roughness_factor   = float 0.5
	u_metallic_factor    = float 0.5
}
@stage: vert {
	#include <common/vert.glsl>
}
@stage: frag {
	#include <common/frag_attrs.glsl>
	#include <common/lighting.glsl>

	void main() {

		// Inputs
		vec3 normal_map = texture(s_normal, v_texcoord0).rgb * 2.0 - 1.0;

		vec3 N = get_normal_from_map(v_world_position, normalize(v_normal), normalize(v_tangent), normal_map);
		vec3 V = normalize(u_cam_position.xyz - v_world_position);
		vec3 L = normalize(u_sun_direction.xyz);
		vec3 H = normalize(V + L);

		// Base material inputs
		vec4 base_color = texture(s_base_color, v_texcoord0);
		base_color.rgb = pow(base_color.rgb, vec3(2.2));
		base_color = base_color * v_color * u_base_color_factor;
		vec2 rm = texture(s_metallic_roughness, v_texcoord0).gb;
		rm = pow(rm, vec2(2.2));
		float roughness = clamp(rm.x * u_roughness_factor, 0.04, 1.0);
		float metallic  = rm.y * u_metallic_factor;

		// Light color
		vec3 light_color = u_sun_intensity.xyz;

		// Fresnel reflectance at normal incidence
		vec3 F0 = mix(vec3(0.04), base_color.rgb, metallic);

		// Cook-Torrance BRDF
		float NDF = distribution_ggx(N, H, roughness);
		float G   = geometry_smith(N, V, L, roughness);
		vec3  F   = fresnel_schlick(max(dot(H, V), 0.0), F0);

		vec3 numerator    = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-5;
		vec3 specular     = numerator / denominator;

		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;

		float NdotL = max(dot(N, L), 0.0);
		vec3 radiance = light_color * NdotL;

		vec3 L_diffuse = (kD * base_color.rgb / PI) * radiance;
		vec3 L_specular = specular * radiance;

		// Simple ambient term
		vec3 ambient = 0.03 * base_color.rgb * u_sun_intensity.xyz;

		// Emissive term
		vec3 emissive = texture(s_emissive, v_texcoord0).rgb;
		emissive = pow(emissive, vec3(2.2));

		// Occlusion term
		float ao = texture(s_occlusion, v_texcoord0).r;

		vec3 result = (ambient + L_diffuse) * ao + L_specular + emissive;
		frag_color = vec4(result, base_color.a);
	}
}