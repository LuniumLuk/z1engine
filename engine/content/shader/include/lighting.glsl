vec4 lambert_shading(vec3 normal, vec4 color) {
	vec3 sun_dir = normalize(u_sun_direction.xyz);
	float NoL = max(dot(normal, sun_dir), 0.0);
	return vec4(color.rgb * NoL, color.a);
}

// Note: phone_shading is superseded by calculate_pbr_illumination for the default
// material (MI_phone / phone.glsl forward path) to maintain lighting parity with
// the deferred pipeline. Kept here for any other callers.
vec4 phone_shading(vec3 normal, vec3 world_pos, vec4 color, float shadow) {
	// Ambient lighting
	vec3 ambient = color.rgb * u_sun_ambient.rgb;

	vec3 total_diffuse = vec3(0.0);
	vec3 total_specular = vec3(0.0);
	vec3 view_dir = normalize(u_cam_position.xyz - world_pos);

	// Sun lighting
	{
		// Diffuse lighting
		vec3 sun_dir = normalize(u_sun_direction.xyz);
		float diff = max(dot(normal, sun_dir), 0.0);
		vec3 diffuse = diff * u_sun_intensity.xyz;

		// Specular lighting
		float specular_strength = 0.5;
		vec3 view_dir = normalize(u_cam_position.xyz - world_pos);
		vec3 refl_dir = reflect(-sun_dir, normal);
		float spec = pow(max(dot(view_dir, refl_dir), 0.0), 32);
		vec3 specular = specular_strength * spec * u_sun_intensity.xyz;

		total_diffuse += diffuse * shadow;
		total_specular += specular * shadow;
	}

	// Loop over lights
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
			vec3 dist_vec = light.position.xyz - world_pos;
			float dist = length(dist_vec);
			if (dist > light.direction.w) continue; // Range check
			light_dir = normalize(dist_vec);

			// Linear falloff based on range
			attenuation = max(0.0, 1.0 - dist / light.direction.w);
			attenuation *= attenuation;

			if (type == 2) { // Spot
				float theta = dot(light_dir, normalize(-light.direction.xyz));
				float inner = light.cone.x;
				float outer = light.cone.y;
				float epsilon = inner - outer;
				// if inner=20, outer=30 (cosines are 0.94, 0.86), epsilon = 0.08
				// if theta > inner (0.94), clamp gives 1.
				// if theta < outer (0.86), clamp gives 0.
				float intensity = clamp((theta - outer) / (epsilon + 1e-5), 0.0, 1.0);
				attenuation *= intensity;
			}
		}

		// Shadows
		float shadow_factor = 1.0;
		if (light.cone.z > 0.5) { // Cast shadows
			// Only apply shadow map if this is a directional light matching the shadow caster
			// We compare light direction with global sun direction (which is used for shadow map generation)
			if (type == 0) {
				vec3 sun_dir = normalize(u_sun_direction.xyz);
				if (dot(light_dir, sun_dir) > 0.99) {
					shadow_factor = shadow;
				}
			}
		}

		// Diffuse
		float diff = max(dot(normal, light_dir), 0.0);
		total_diffuse += diff * light.color.rgb * light.color.w * attenuation * shadow_factor;

		// Specular
		float specular_strength = 0.5;
		vec3 refl_dir = reflect(-light_dir, normal);
		float spec = pow(max(dot(view_dir, refl_dir), 0.0), 32);
		total_specular += specular_strength * spec * light.color.rgb * light.color.w * attenuation * shadow_factor;
	}

	vec3 result = (ambient + total_diffuse + total_specular) * color.rgb;
	return vec4(result, color.a);
}

// ----------------------------------------------------------------------------
// GGX/Schlick/Smith helpers
// ----------------------------------------------------------------------------

const float PI = 3.14159265359;

float distribution_ggx(vec3 N, vec3 H, float roughness) {
	float a      = roughness * roughness;
	float a2     = a * a;
	float NoH  = max(dot(N, H), 0.0);
	float NoH2 = NoH * NoH;

	float denom = (NoH2 * (a2 - 1.0) + 1.0);
	return a2 / (PI * denom * denom + 1e-5);
}

float geometry_schlick_ggx(float NoV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;
	return NoV / (NoV * (1.0 - k) + k);
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NoV = max(dot(N, V), 0.0);
	float NoL = max(dot(N, L), 0.0);
	float ggx1 = geometry_schlick_ggx(NoV, roughness);
	float ggx2 = geometry_schlick_ggx(NoL, roughness);
	return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cos_theta, vec3 f0) {
	return f0 + (1.0 - f0) * pow(1.0 - cos_theta, 5.0);
}

void calculate_pbr_illumination(
	vec3 light_dir, vec3 light_color, float attenuation, float shadow_factor,
	vec3 N, vec3 V, vec3 F0, float roughness, float metallic, vec3 base_color,
	inout vec3 L_diffuse, inout vec3 L_specular
) {
	vec3 L = light_dir;
	vec3 H = normalize(V + L);

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
	vec3 radiance = light_color * attenuation; // Light color includes intensity

	L_diffuse += (kD * base_color / PI) * radiance * NdotL * shadow_factor;
	L_specular += specular * radiance * NdotL * shadow_factor;
}

// ----------------------------------------------------------------------------
// Normal mapping with fallback
// ----------------------------------------------------------------------------

vec3 get_normal_from_map(vec3 world_pos, vec3 normal, vec4 tangent, vec3 normal_map) {
	// Case 1: valid tangent provided (glTF format)
	if (abs(tangent.w) > 0.0) {
		vec3 T = normalize(tangent.xyz);
		vec3 N = normalize(normal);
		vec3 B = normalize(cross(N, T) * tangent.w);
		mat3 TBN = mat3(T, B, N);
		return normalize(TBN * normal_map);
	}

	// Case 2: no tangent - fallback using screen-space derivatives
	vec3 dp1 = dFdx(world_pos);
	vec3 dp2 = dFdy(world_pos);
	vec2 duv1 = dFdx(v_texcoord0);
	vec2 duv2 = dFdy(v_texcoord0);

	if (dot(duv1, duv1) + dot(duv2, duv2) < 1e-12) {
		return normalize(normal);
	}

	vec3 N = normalize(normal);
	vec3 T = normalize(dp1 * duv2.y - dp2 * duv1.y);
	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * normal_map);
}

// ----------------------------------------------------------------------------
// Shadow mapping (PCF / PCSS)
// ----------------------------------------------------------------------------

const float k_light_radius = 0.3;
const float k_max_filter_radius = 6.0;
const float k_blocker_search_radius = 2.5;

float sample_shadow(vec3 uv, float depth, float bias) {
	return depth - bias > texture(u_shadow_map, uv).r ? 0.0 : 1.0;
}

float sample_shadow_pcf(vec3 uv, float depth, float bias) {
	ivec3 tex_size = textureSize(u_shadow_map, 0);

	vec2 texel_pos = uv.xy * vec2(tex_size.xy) - 0.5;
	vec2 frac_pos = fract(texel_pos);
	ivec2 base = ivec2(floor(texel_pos));
	ivec2 next = base + ivec2(1, 1);
	int z = int(uv.z);

	float s00 = texelFetch(u_shadow_map, ivec3(base.x, base.y, z), 0).r < depth - bias ? 0.0 : 1.0;
	float s10 = texelFetch(u_shadow_map, ivec3(next.x, base.y, z), 0).r < depth - bias ? 0.0 : 1.0;
	float s01 = texelFetch(u_shadow_map, ivec3(base.x, next.y, z), 0).r < depth - bias ? 0.0 : 1.0;
	float s11 = texelFetch(u_shadow_map, ivec3(next.x, next.y, z), 0).r < depth - bias ? 0.0 : 1.0;

	return mix(mix(s00, s10, frac_pos.x), mix(s01, s11, frac_pos.x), frac_pos.y);
}

float pcf_filter(vec3 uv, float depth, vec2 texel_size, float radius, float bias) {
	float shadow = 0.0;
	float samples = 0.0;
	vec2 scaled = texel_size * radius;
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			vec2 offset = vec2(float(x), float(y)) * scaled;
			shadow += sample_shadow_pcf(vec3(uv.xy + offset, uv.z), depth, bias);
			samples += 1.0;
		}
	}
	return shadow / samples;
}

float find_blockers(vec3 uv, float depth, vec2 texel_size, float bias) {
	float sum = 0.0;
	float count = 0.0;
	vec2 scaled = texel_size * k_blocker_search_radius;
	for (int x = -2; x <= 2; ++x) {
		for (int y = -2; y <= 2; ++y) {
			vec2 offset = vec2(float(x), float(y)) * scaled;
			float sampled = texture(u_shadow_map, vec3(uv.xy + offset, uv.z)).r;
			if (sampled < depth - bias) {
				sum += sampled;
				count += 1.0;
			}
		}
	}
	return count > 0.0 ? sum / count : 0.0;
}

int get_cascade_index(vec3 world_pos) {
	float dist = distance(u_cam_position.xyz, world_pos);
	if (dist < u_csm_splits.x) return 0;
	if (dist < u_csm_splits.y) return 1;
	if (dist < u_csm_splits.z) return 2;
	return 3;
}

float get_shadow() {
	int layer = get_cascade_index(v_world_position);

	vec4 ls = u_sun_projview[layer] * vec4(v_world_position, 1.0);
	vec3 proj = ls.xyz / ls.w;
	vec3 uv = vec3(proj.xy * 0.5 + 0.5, float(layer));
	float current_depth = proj.z * 0.5 + 0.5;

	vec3 N = normalize(v_normal);
	vec3 L = normalize(u_sun_direction.xyz);
	float cos_theta = clamp(dot(N, L), 0.0, 1.0);
	float bias = max(0.005 * (1.0 - cos_theta), 0.002);
	if (layer > 0) bias *= 0.5; // reduce bias for further cascades since the depth range covers more world space

	// Check if point is outside the shadow map bounds
	// if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
	// 	return 1.0;
	// }

	vec2 texel_size = 1.0 / vec2(textureSize(u_shadow_map, 0).xy);
	float blockers = find_blockers(uv, current_depth, texel_size, bias);
	if (blockers == 0.0) {
		return pcf_filter(uv, current_depth, texel_size, 1.0, bias);
	}

	float diff = max(0.0, current_depth - blockers);
	float penumbra = diff / max(blockers, 1e-4) * k_light_radius;
	penumbra = clamp(penumbra, 0.0, k_max_filter_radius);
	float filter_radius = max(1.0, penumbra);
	return pcf_filter(uv, current_depth, texel_size, filter_radius, bias);
}
