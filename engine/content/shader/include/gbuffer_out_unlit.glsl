// G-buffer MRT output for unlit materials
// Writes albedo with metallic=0, roughness=1 (non-reflective)
// Include this in the fragment stage of unlit.glsl inside #ifdef VARIANT_GBUFFER

layout(location = 1) out vec4 gbuffer_normal;
layout(location = 2) out vec4 gbuffer_albedo;
layout(location = 3) out vec4 gbuffer_metallic_roughness;
layout(location = 4) out vec4 gbuffer_emissive;

void main() {
	vec4 color = v_color * texture(s_base_color, v_texcoord0);

	vec3 N = normalize(v_normal);

	// Unlit: no metallic, maximum roughness
	float metallic = 0.0;
	float roughness = 1.0;

	// Write G-buffer
	frag_color = vec4(v_world_position, 1.0);
	gbuffer_normal = vec4(N, 0.0);
	gbuffer_albedo = vec4(color.rgb, 1.0);
	gbuffer_metallic_roughness = vec4(metallic, roughness, 0.0, 0.0);
	gbuffer_emissive = vec4(0.0, 0.0, 0.0, 0.0);
}
