// G-buffer MRT output for Phong materials
// Approximates PBR metallic-roughness values from Phong parameters
// Include this in the fragment stage of phone.glsl inside #ifdef VARIANT_GBUFFER

layout(location = 1) out vec4 gbuffer_normal;
layout(location = 2) out vec4 gbuffer_albedo;
layout(location = 3) out vec4 gbuffer_metallic_roughness;
layout(location = 4) out vec4 gbuffer_emissive;

void main() {
	vec4 color = v_color * texture(s_base_color, v_texcoord0);

	if (u_alpha_mode == 0) {
		color.a = 1.0;
	}
	else if (u_alpha_mode == 1) {
		if (color.a < u_alpha_cutoff) discard;
	}

	vec3 N = normalize(v_normal);

	// Phong -> PBR approximation: non-metallic, moderate roughness
	float metallic = 0.0;
	float roughness = 0.5;

	// Write G-buffer
	frag_color = vec4(v_world_position, color.a);
	gbuffer_normal = vec4(N, 0.0);
	gbuffer_albedo = vec4(color.rgb, color.a);
	gbuffer_metallic_roughness = vec4(metallic, roughness, 0.0, 0.0);
	gbuffer_emissive = vec4(0.0, 0.0, 0.0, 0.0);
}
