	uniform sampler2D s_normal;
	uniform int u_normal_uv_set;

	uniform sampler2D s_emissive;
	uniform int u_emissive_uv_set;
	uniform float u_emissive_factor;

	uniform sampler2D s_occlusion;
	uniform int u_occlusion_uv_set;

#ifdef SPECULAR_GLOSSINESS

	uniform sampler2D s_diffuse;
	uniform vec4 u_diffuse_factor;

	uniform sampler2D s_specular_glossiness;
	uniform vec3 u_specular_factor;
	uniform float u_glossiness_factor;

#else

	uniform sampler2D s_base_color;
	uniform int u_base_color_uv_set;
	uniform vec4 u_base_color_factor;

	uniform sampler2D s_metallic_roughness;
	uniform int u_metallic_roughness_uv_set;
	uniform float u_metallic_factor;
	uniform float u_roughness_factor;

#endif
