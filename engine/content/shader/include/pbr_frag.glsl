	vec2 get_uv(int uv_set) {
		if (uv_set == 1) return v_texcoord1;
		return v_texcoord0;
	}

	void main() {

		// Inputs
		vec3 normal_map = texture(s_normal, get_uv(u_normal_uv_set)).rgb * 2.0 - 1.0;

		vec3 N = get_normal_from_map(v_world_position, v_normal, v_tangent, normal_map);
		vec3 V = normalize(u_cam_position.xyz - v_world_position);

		float shadow = get_shadow();

#ifdef SPECULAR_GLOSSINESS

		// Diffuse
		vec4 diffuse_sample = texture(s_diffuse, v_texcoord0);
		diffuse_sample.rgb = pow(diffuse_sample.rgb, vec3(2.2));
		vec4 diffuse_color_vec = diffuse_sample * v_color * u_diffuse_factor;
		vec3 base_color = diffuse_color_vec.rgb;
		float alpha = diffuse_color_vec.a;

#else

		// Base material inputs
		vec4 base_color_sample = texture(s_base_color, get_uv(u_base_color_uv_set));
		base_color_sample.rgb = pow(base_color_sample.rgb, vec3(2.2));
		vec4 base_color_vec = base_color_sample * v_color * u_base_color_factor;
		vec3 base_color = base_color_vec.rgb;
		float alpha = base_color_vec.a;

#endif

		if (u_alpha_mode == 0) {
			alpha = 1.0;
		}
		else if (u_alpha_mode == 1) {
			if (alpha < u_alpha_cutoff) discard;
		}

#ifdef SPECULAR_GLOSSINESS

		// Specular Glossiness
		vec4 sg_sample = texture(s_specular_glossiness, v_texcoord0);
		sg_sample.rgb = pow(sg_sample.rgb, vec3(2.2)); // Specular color is also in sRGB usually? GLTF spec says so.

		vec3 specular_color = sg_sample.rgb * u_specular_factor;
		float glossiness = sg_sample.a * u_glossiness_factor;
		float roughness = 1.0 - glossiness;
		roughness = clamp(roughness, 0.04, 1.0);

		// F0 is directly the specular color
		vec3 F0 = specular_color;

		// Metallic is effectively 0 for the purpose of the generic lighting function
		// because we are providing F0 and Diffuse Color explicitly.
		// However, calculate_pbr_illumination uses metallic to scale kD.
		// kD = (1 - kS) * (1 - metallic)
		// We want kD = (1 - kS) * fraction_avail_for_diffuse?
		// In SG workflow, the diffuse color provided IS the albedo.
		// Energy conservation says: Diffuse + Specular <= 1.
		// The generic function does:
		// L_diffuse += (kD * base_color / PI) ...
		// If we set metallic = 0, kD = (1-kS).
		// So Diffuse = (1-F) * DiffuseColor / PI.
		// This matches the PBR equation logic.
		float metallic = 0.0;

#else

		vec2 rm = texture(s_metallic_roughness, get_uv(u_metallic_roughness_uv_set)).gb;
		rm = pow(rm, vec2(2.2));
		float roughness = clamp(rm.x * u_roughness_factor, 0.04, 1.0);
		float metallic  = rm.y * u_metallic_factor;

		// Fresnel reflectance at normal incidence
		vec3 F0 = mix(vec3(0.04), base_color, metallic);

#endif

		vec3 L_diffuse = vec3(0.0);
		vec3 L_specular = vec3(0.0);

		// Sunlight
		calculate_pbr_illumination(normalize(u_sun_direction.xyz), u_sun_intensity.rgb, 1.0, shadow,
			N, V, F0, roughness, metallic, base_color, L_diffuse, L_specular);

		int count = int(u_lights_count.x);
		for (int i = 0; i < count; ++i) {
			Light light = u_lights[i];
			vec3 light_dir;
			float attenuation = 1.0;

			// 0: Directional, 1: Point, 2: Spot
			int type = int(light.position.w);

			if (type == 0) { // Directional
				light_dir = normalize(-light.direction.xyz);
			}
			else { // Point or Spot
				vec3 dist_vec = light.position.xyz - v_world_position;
				float dist = length(dist_vec);
				if (dist > light.direction.w) continue; // Range check
				light_dir = normalize(dist_vec);

				// Linear falloff
				attenuation = max(0.0, 1.0 - dist / light.direction.w);
				attenuation *= attenuation;

				if (type == 2) { // Spot
					float theta = dot(light_dir, normalize(-light.direction.xyz));
					float inner = light.cone.x;
					float outer = light.cone.y;
					float epsilon = inner - outer;
					float intensity = clamp((theta - outer) / (epsilon + 1e-5), 0.0, 1.0);
					attenuation *= intensity;
				}
			}

			// Shadows
			float shadow_factor = 1.0;
			if (light.cone.z > 0.5) { // Cast shadows
				if (type == 0) {
					vec3 sun_dir = normalize(u_sun_direction.xyz);
					if (dot(light_dir, sun_dir) > 0.99) {
						shadow_factor = shadow;
					}
				}
			}

			calculate_pbr_illumination(light_dir, light.color.rgb * light.color.w, attenuation, shadow_factor,
				N, V, F0, roughness, metallic, base_color, L_diffuse, L_specular);
		}

		// Simple ambient term
		vec3 ambient = base_color * u_sun_ambient.rgb;

		// Emissive term
		vec3 emissive = texture(s_emissive, get_uv(u_emissive_uv_set)).rgb;
		emissive = pow(emissive, vec3(2.2)) * u_emissive_factor;

		// Occlusion term
		float ao = texture(s_occlusion, get_uv(u_occlusion_uv_set)).r;

		vec3 result = (ambient + L_diffuse) * ao + L_specular + emissive;
		frag_color = vec4(result, alpha);

		frag_color = vec4(
			isnan(frag_color.x) ? 0.0 : frag_color.x,
			isnan(frag_color.y) ? 0.0 : frag_color.y,
			isnan(frag_color.z) ? 0.0 : frag_color.z,
			isnan(frag_color.w) ? 0.0 : frag_color.w
		);
	}
