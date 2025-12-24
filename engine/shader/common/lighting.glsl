vec4 lambert_shading(vec3 normal, vec4 color) {
	vec3 sun_dir = normalize(u_sun_direction.xyz);
	float NoL = max(dot(normal, sun_dir), 0.0);
	return vec4(color.rgb * NoL, color.a);
}

vec4 phone_shading(vec3 normal, vec3 world_pos, vec4 color) {
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

	vec3 result = (ambient + diffuse + specular) * color.rgb;
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