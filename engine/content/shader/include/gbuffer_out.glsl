// G-buffer MRT output for PBR metallic-roughness materials
// Include this in the fragment stage of a surface shader inside #ifdef VARIANT_GBUFFER

layout(location = 1) out vec4 gbuffer_normal;
layout(location = 2) out vec4 gbuffer_albedo;
layout(location = 3) out vec4 gbuffer_metallic_roughness;
layout(location = 4) out vec4 gbuffer_emissive;

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
	vec3 N = get_normal_from_map(v_world_position, v_normal, v_tangent, normal_map);

	// Metallic / roughness
	vec2 rm = texture(s_metallic_roughness, get_uv(u_metallic_roughness_uv_set)).gb;
	rm = pow(rm, vec2(2.2));
	float roughness = clamp(rm.x * u_roughness_factor, 0.04, 1.0);
	float metallic  = rm.y * u_metallic_factor;

	// Emissive
	vec3 emissive = texture(s_emissive, get_uv(u_emissive_uv_set)).rgb;
	emissive = pow(emissive, vec3(2.2)) * u_emissive_factor;

	// Write G-buffer:
	// RT0 (frag_color): world position + alpha
	frag_color = vec4(v_world_position, alpha);
	// RT1: world normal
	gbuffer_normal = vec4(N, 0.0);
	// RT2: albedo (linear-space base color)
	gbuffer_albedo = vec4(base_color_vec.rgb, alpha);
	// RT3: metallic (R), roughness (G)
	gbuffer_metallic_roughness = vec4(metallic, roughness, 0.0, 0.0);
	// RT4: emissive
	gbuffer_emissive = vec4(emissive, 0.0);

}
