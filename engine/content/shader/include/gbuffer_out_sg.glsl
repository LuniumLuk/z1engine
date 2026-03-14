// G-buffer MRT output for PBR specular-glossiness materials
// Converts SG parameters to metallic-roughness for G-buffer storage
// Include this in the fragment stage of pbr_sg.glsl inside #ifdef VARIANT_GBUFFER

layout(location = 1) out vec4 gbuffer_normal;
layout(location = 2) out vec4 gbuffer_albedo;
layout(location = 3) out vec4 gbuffer_metallic_roughness;
layout(location = 4) out vec4 gbuffer_emissive;

vec2 get_uv_sg(int uv_set) {
	if (uv_set == 1) return v_texcoord1;
	return v_texcoord0;
}

void main() {
	// Diffuse
	vec4 diffuse_sample = texture(s_diffuse, v_texcoord0);
	diffuse_sample.rgb = pow(diffuse_sample.rgb, vec3(2.2));
	vec4 diffuse_color_vec = diffuse_sample * v_color * u_diffuse_factor;
	vec3 base_color = diffuse_color_vec.rgb;
	float alpha = diffuse_color_vec.a;

	if (u_alpha_mode == 0) {
		alpha = 1.0;
	}
	else if (u_alpha_mode == 1) {
		if (alpha < u_alpha_cutoff) discard;
	}

	// Normal mapping
	vec3 normal_map = texture(s_normal, get_uv_sg(u_normal_uv_set)).rgb * 2.0 - 1.0;
	vec3 N = get_normal_from_map(v_world_position, v_normal, v_tangent, normal_map);

	// Specular-glossiness to metallic-roughness conversion
	vec4 sg_sample = texture(s_specular_glossiness, v_texcoord0);
	sg_sample.rgb = pow(sg_sample.rgb, vec3(2.2));
	vec3 specular_color = sg_sample.rgb * u_specular_factor;
	float glossiness = sg_sample.a * u_glossiness_factor;
	float roughness = clamp(1.0 - glossiness, 0.04, 1.0);

	// Approximate metallic from specular color luminance
	float specular_lum = dot(specular_color, vec3(0.2126, 0.7152, 0.0722));
	float diffuse_lum = dot(base_color, vec3(0.2126, 0.7152, 0.0722));
	float metallic = clamp(specular_lum / (specular_lum + diffuse_lum + 0.001), 0.0, 1.0);
	vec3 emissive = texture(s_emissive, get_uv_sg(u_emissive_uv_set)).rgb;
	emissive = pow(emissive, vec3(2.2)) * u_emissive_factor;

	// Write G-buffer
	frag_color = vec4(v_world_position, alpha);
	gbuffer_normal = vec4(N, 0.0);
	gbuffer_albedo = vec4(base_color, alpha);
	gbuffer_metallic_roughness = vec4(metallic, roughness, 0.0, 0.0);
	gbuffer_emissive = vec4(emissive, 0.0);

}
