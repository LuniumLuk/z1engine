vec4 lambert_shading(vec3 normal, vec4 color) {
	vec3 sun_dir = normalize(u_sun_direction.xyz);
	float NoL = max(dot(normal, sun_dir), 0.0);
	return vec4(color.rgb * NoL, color.a);
}

vec4 phone_shading(vec3 normal, vec3 world_pos, vec4 color, float shadow) {
	// Ambient lighting
	vec3 ambient = color.rgb * u_sun_ambient.rgb;

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

	vec3 result = (ambient + (diffuse + specular) * shadow) * color.rgb;
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

	// Case 2: no tangent — fallback using screen-space derivatives
	vec3 dp1 = dFdx(world_pos);
	vec3 dp2 = dFdy(world_pos);
	vec2 duv1 = dFdx(v_texcoord0);
	vec2 duv2 = dFdy(v_texcoord0);

	vec3 N = normalize(normal);
	vec3 T = normalize(dp1 * duv2.y - dp2 * duv1.y);
	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * normal_map);
}

// ----------------------------------------------------------------------------
// Shadow mapping (PCF / PCSS)
// ----------------------------------------------------------------------------

const float k_shadow_bias = 0.005;
const float k_light_radius = 0.3;
const float k_max_filter_radius = 6.0;
const float k_blocker_search_radius = 2.5;

float sample_shadow(vec2 uv, vec2 offset, float depth) {
	return depth - k_shadow_bias > texture(u_shadow_map, uv + offset).r ? 0.0 : 1.0;
}

float pcf_filter(vec2 uv, float depth, vec2 texel_size, float radius) {
	float shadow = 0.0;
	float samples = 0.0;
	vec2 scaled = texel_size * radius;
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			vec2 offset = vec2(float(x), float(y)) * scaled;
			shadow += sample_shadow(uv, offset, depth);
			samples += 1.0;
		}
	}
	return shadow / samples;
}

float find_blockers(vec2 uv, float depth, vec2 texel_size) {
	float sum = 0.0;
	float count = 0.0;
	vec2 scaled = texel_size * k_blocker_search_radius;
	for (int x = -2; x <= 2; ++x) {
		for (int y = -2; y <= 2; ++y) {
			vec2 offset = vec2(float(x), float(y)) * scaled;
			float sampled = texture(u_shadow_map, uv + offset).r;
			if (sampled < depth - k_shadow_bias) {
				sum += sampled;
				count += 1.0;
			}
		}
	}
	return count > 0.0 ? sum / count : 0.0;
}

float get_shadow() {
	vec4 ls = u_sun_projview * vec4(v_world_position, 1.0);
	vec3 proj = ls.xyz / ls.w;
	vec2 uv = proj.xy * 0.5 + 0.5;
	float current_depth = proj.z * 0.5 + 0.5;
	if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
		return 1.0;
	}

	vec2 texel_size = 1.0 / vec2(textureSize(u_shadow_map, 0));
	float blockers = find_blockers(uv, current_depth, texel_size);
	if (blockers == 0.0) {
		return pcf_filter(uv, current_depth, texel_size, 1.0);
	}

	float diff = max(0.0, current_depth - blockers);
	float penumbra = diff / max(blockers, 1e-4) * k_light_radius;
	penumbra = clamp(penumbra, 0.0, k_max_filter_radius);
	float filter_radius = max(1.0, penumbra);
	return pcf_filter(uv, current_depth, texel_size, filter_radius);
}
