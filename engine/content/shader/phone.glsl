@uniforms: {
	#include <include/uniforms.glsl>

	uniform sampler2D s_base_color;
}
@reflections: {
	#include <include/reflections.glsl>

	s_base_color = sampler2D texture/T_white
}
@variants: {
	VARIANT_GBUFFER
	VARIANT_SHADOW
	VARIANT_VELOCITY
}
@stage: vert {
	#include <include/vert.glsl>
}
@stage: frag {
	#include <include/frag_attrs.glsl>

#ifdef VARIANT_GBUFFER
	#include <include/gbuffer_out_phone.glsl>
#elif defined(VARIANT_SHADOW)
	#include <include/shadow_out.glsl>
#elif defined(VARIANT_VELOCITY)
	#include <include/velocity_out.glsl>
#else
	#include <include/lighting.glsl>

	void main() {
		vec4 color = v_color * texture(s_base_color, v_texcoord0);
		if (u_alpha_mode == 0) {
			color.a = 1.0;
		}
		else if (u_alpha_mode == 1) {
			if (color.a < u_alpha_cutoff) discard;
		}

		// Use PBR lighting to match the deferred pipeline output for this material.
		// Constants mirror gbuffer_out_phone.glsl so both paths produce the same result.
		float metallic  = 0.0;
		float roughness = 0.5;
		vec3 base_color = color.rgb;
		vec3 N = normalize(v_normal);
		vec3 V = normalize(u_cam_position.xyz - v_world_position);
		vec3 F0 = mix(vec3(0.04), base_color, metallic);
		float shadow = get_shadow();

		vec3 L_diffuse  = vec3(0.0);
		vec3 L_specular = vec3(0.0);

		// Sun
		calculate_pbr_illumination(
			normalize(u_sun_direction.xyz), u_sun_intensity.rgb, 1.0, shadow,
			N, V, F0, roughness, metallic, base_color,
			L_diffuse, L_specular);

		// Dynamic lights
		int count = int(u_lights_count.x);
		for (int i = 0; i < count; ++i) {
			Light light = u_lights[i];
			vec3 light_dir;
			float attenuation = 1.0;

			int type = int(light.position.w);
			if (type == 0) {
				light_dir = normalize(-light.direction.xyz);
			}
			else {
				vec3 dist_vec = light.position.xyz - v_world_position;
				float dist = length(dist_vec);
				if (dist > light.direction.w) continue;
				light_dir = normalize(dist_vec);
				attenuation = max(0.0, 1.0 - dist / light.direction.w);
				attenuation *= attenuation;
				if (type == 2) {
					float theta   = dot(light_dir, normalize(-light.direction.xyz));
					float inner   = light.cone.x;
					float outer   = light.cone.y;
					float epsilon = inner - outer;
					attenuation  *= clamp((theta - outer) / (epsilon + 1e-5), 0.0, 1.0);
				}
			}

			float shadow_factor = 1.0;
			if (light.cone.z > 0.5 && type == 0) {
				if (dot(normalize(-light.direction.xyz), normalize(u_sun_direction.xyz)) > 0.99)
					shadow_factor = shadow;
			}

			calculate_pbr_illumination(
				light_dir, light.color.rgb * light.color.w, attenuation, shadow_factor,
				N, V, F0, roughness, metallic, base_color,
				L_diffuse, L_specular);
		}

		vec3 ambient = base_color * u_sun_ambient.rgb;
		frag_color = vec4(ambient + L_diffuse + L_specular, color.a);
	}
#endif
}
