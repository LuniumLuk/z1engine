@uniforms: {
	#include <include/uniforms.glsl>

	uniform sampler2D u_gbuffer_position;
	uniform sampler2D u_gbuffer_normal;
	uniform sampler2D u_gbuffer_albedo;
	uniform sampler2D u_gbuffer_metallic_roughness;
	uniform sampler2D u_gbuffer_emissive;

#ifdef VARIANT_MSAA_EDGE
	// Per-sample G-buffer access for sample-frequency edge shading (MSAA).
	uniform sampler2DMS u_gbuffer_position_ms;
	uniform sampler2DMS u_gbuffer_normal_ms;
	uniform sampler2DMS u_gbuffer_albedo_ms;
	uniform sampler2DMS u_gbuffer_metallic_roughness_ms;
	uniform sampler2DMS u_gbuffer_emissive_ms;
	uniform sampler2DMS u_gbuffer_depth_ms;
#endif
}
@reflections: {
	u_gbuffer_position           [invisible]
	u_gbuffer_normal             [invisible]
	u_gbuffer_albedo             [invisible]
	u_gbuffer_metallic_roughness [invisible]
	u_gbuffer_emissive           [invisible]
	u_shadow_map                 [invisible]
	u_ao_texture                 [invisible]
}
@variants: {
	VARIANT_MSAA_EDGE
}
@stage: vert {
	#include <include/quad.glsl>
}
@stage: frag {
	layout(location = 0) out vec4 frag_color;
	layout(location = 0) in vec2 v_uv;

	// Provide variables that lighting.glsl's get_shadow() expects
	vec3 v_world_position;
	vec3 v_normal;
	vec2 v_texcoord0;

	#include <include/lighting.glsl>

	// Full deferred shading of one surface point: called once per pixel by the bulk pass and
	// once per sample by the MSAA edge pass (VARIANT_MSAA_EDGE).
	vec4 shade_deferred(vec3 world_pos, float alpha, vec3 emissive, vec3 N, vec3 base_color, vec2 mr, vec2 uv) {
		// Empty pixels carry sky emissive from the deferred sky pass; preserve it.
		if (alpha == 0.0) {
			return vec4(emissive, 1.0);
		}

		float metallic = mr.r;
		float roughness = mr.g;

		// Set up variables expected by lighting functions
		v_world_position = world_pos;
		v_normal = N;
		v_texcoord0 = uv; // used by get_normal_from_map fallback (not called here, but satisfy the reference)

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

		calculate_sky_ibl(N, V, F0, roughness, metallic, base_color, L_diffuse, L_specular);

		// Ambient
		vec3 ambient = base_color * u_sun_ambient.rgb;

		// Screen-space ambient occlusion (bound by the renderer when enabled)
		float ao_screen = u_ao_enabled > 0.5 ? texture(u_ao_texture, uv).r : 1.0;

		vec3 result = (ambient * ao_screen + L_diffuse) + L_specular + emissive;
		vec4 color = vec4(result, alpha);

		// NaN guard
		color = vec4(
			isnan(color.x) ? 0.0 : color.x,
			isnan(color.y) ? 0.0 : color.y,
			isnan(color.z) ? 0.0 : color.z,
			isnan(color.w) ? 0.0 : color.w
		);
		return color;
	}

#ifdef VARIANT_MSAA_EDGE
	// Segments a pixel as an edge when its samples do not belong to the same surface. Samples of
	// one surface are spatially contiguous and their normals agree; silhouettes, empty samples and
	// creases (coincident positions, divergent normals) are edges.
	bool sample_is_edge(ivec2 coord) {
		vec3 position0 = texelFetch(u_gbuffer_position_ms, coord, 0).xyz;
		vec3 normal0 = texelFetch(u_gbuffer_normal_ms, coord, 0).xyz;
		ivec2 tex_size = textureSize(u_gbuffer_depth_ms);
		// World size of one pixel at the reference sample's distance (2 / proj[1][1] = 2*tan(fov/2))
		float world_per_pixel = 2.0 / u_projview[1][1] / float(tex_size.y);
		float ref_distance = max(length(position0 - u_cam_position.xyz), 1.0);
		// 4x margin over the intra-pixel sample spacing; strongly foreshortened surfaces may
		// classify as edges, which only costs shading time (the result stays correct).
		float limit = 4.0 * world_per_pixel * ref_distance;
		bool has_normal0 = dot(normal0, normal0) > 0.5; // empty (sky) samples carry no normal

		for (int s = 1; s < gl_NumSamples; ++s) {
			vec3 position_s = texelFetch(u_gbuffer_position_ms, coord, s).xyz;
			if (distance(position_s, position0) > limit) {
				return true;
			}
			// Creases and hard normal seams: adjacent faces meet at (nearly) the same position,
			// but their normals diverge; the resolved average normal would be invalid for lighting.
			vec3 normal_s = texelFetch(u_gbuffer_normal_ms, coord, s).xyz;
			if (has_normal0 && dot(normal_s, normal_s) > 0.5 &&
				dot(normalize(normal0), normalize(normal_s)) < 0.98) {
				return true;
			}
		}
		return false;
	}

	// Sample-frequency pass: shades only the samples of partially covered pixels. Using
	// gl_SampleID makes this program execute once per covered sample; non-edge pixels discard.
	void main() {
		ivec2 coord = ivec2(gl_FragCoord.xy);
		if (!sample_is_edge(coord)) {
			discard;
		}

		int sample_id = gl_SampleID;
		gl_SampleMask[0] = 1 << sample_id;

		vec4 position_alpha = texelFetch(u_gbuffer_position_ms, coord, sample_id);
		vec3 emissive = texelFetch(u_gbuffer_emissive_ms, coord, sample_id).rgb;
		vec3 N = normalize(texelFetch(u_gbuffer_normal_ms, coord, sample_id).xyz);
		vec3 base_color = texelFetch(u_gbuffer_albedo_ms, coord, sample_id).rgb;
		vec2 mr = texelFetch(u_gbuffer_metallic_roughness_ms, coord, sample_id).rg;

		frag_color = shade_deferred(position_alpha.xyz, position_alpha.w, emissive, N, base_color, mr, v_uv);
	}
#else
	void main() {
		vec4 position_alpha = texture(u_gbuffer_position, v_uv);
		vec3 emissive = texture(u_gbuffer_emissive, v_uv).rgb;
		vec3 N = normalize(texture(u_gbuffer_normal, v_uv).xyz);
		vec3 base_color = texture(u_gbuffer_albedo, v_uv).rgb;
		vec2 mr = texture(u_gbuffer_metallic_roughness, v_uv).rg;

		frag_color = shade_deferred(position_alpha.xyz, position_alpha.w, emissive, N, base_color, mr, v_uv);
	}
#endif
}
