@uniforms: {
	#include <common/uniforms.glslh>

	uniform sampler2D u_gbuffer_position;
	uniform sampler2D u_gbuffer_normal;
	uniform sampler2D u_gbuffer_albedo;
	uniform sampler2D u_gbuffer_metallic_roughness;
}
@reflections: {
	u_gbuffer_position           [invisible]
	u_gbuffer_normal             [invisible]
	u_gbuffer_albedo             [invisible]
	u_gbuffer_metallic_roughness [invisible]
	u_shadow_map                 [invisible]
}
@stage: vert {
	#include <common/quad.glslh>
}
@stage: frag {
	layout(location = 0) out vec4 frag_color;
	layout(location = 0) in vec2 v_uv;

	// Provide variables that lighting.glslh's get_shadow() expects
	vec3 v_world_position;
	vec3 v_normal;
	vec2 v_texcoord0;

	#include <common/lighting.glslh>

	void main() {
		// Sample G-buffer
		vec4 position_alpha = texture(u_gbuffer_position, v_uv);
		vec3 world_pos = position_alpha.xyz;
		float alpha = position_alpha.w;

		// Early discard for empty pixels (no geometry was written here)
		if (alpha == 0.0) discard;

		vec3 N = normalize(texture(u_gbuffer_normal, v_uv).xyz);
		vec4 albedo_sample = texture(u_gbuffer_albedo, v_uv);
		vec3 base_color = albedo_sample.rgb;
		vec2 mr = texture(u_gbuffer_metallic_roughness, v_uv).rg;
		float metallic = mr.r;
		float roughness = mr.g;

		// Set up variables expected by lighting functions
		v_world_position = world_pos;
		v_normal = N;
		v_texcoord0 = v_uv; // used by get_normal_from_map fallback (not called here, but satisfy the reference)

		// Shadow
		float shadow = get_shadow();

		// View direction
		vec3 V = normalize(u_cam_position.xyz - world_pos);

		// Fresnel reflectance at normal incidence
		vec3 F0 = mix(vec3(0.04), base_color, metallic);

		vec3 L_diffuse = vec3(0.0);
		vec3 L_specular = vec3(0.0);

		// Sunlight
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
				vec3 dist_vec = light.position.xyz - world_pos;
				float dist = length(dist_vec);
				if (dist > light.direction.w) continue;
				light_dir = normalize(dist_vec);

				attenuation = max(0.0, 1.0 - dist / light.direction.w);
				attenuation *= attenuation;

				if (type == 2) {
					float theta = dot(light_dir, normalize(-light.direction.xyz));
					float inner = light.cone.x;
					float outer = light.cone.y;
					float epsilon = inner - outer;
					float intensity = clamp((theta - outer) / (epsilon + 1e-5), 0.0, 1.0);
					attenuation *= intensity;
				}
			}

			float shadow_factor = 1.0;
			if (light.cone.z > 0.5) {
				if (type == 0) {
					vec3 sun_dir = normalize(u_sun_direction.xyz);
					if (dot(light_dir, sun_dir) > 0.99) {
						shadow_factor = shadow;
					}
				}
			}

			calculate_pbr_illumination(
				light_dir, light.color.rgb * light.color.w, attenuation, shadow_factor,
				N, V, F0, roughness, metallic, base_color,
				L_diffuse, L_specular);
		}

		// Ambient
		vec3 ambient = base_color * u_sun_ambient.rgb;

		vec3 result = (ambient + L_diffuse) + L_specular;
		frag_color = vec4(result, alpha);

		// NaN guard
		frag_color = vec4(
			isnan(frag_color.x) ? 0.0 : frag_color.x,
			isnan(frag_color.y) ? 0.0 : frag_color.y,
			isnan(frag_color.z) ? 0.0 : frag_color.z,
			isnan(frag_color.w) ? 0.0 : frag_color.w
		);
	}
}
