@uniforms: {
	#include <common/uniforms.glsl>
	#include <common/pbr_uniforms.glsl>
}
@reflections: {
	#include <common/reflections.glsl>
	#include <common/pbr_reflections.glsl>

	s_base_color                = sampler2D texture/T_white
	u_base_color_uv_set         = int 0
	u_base_color_factor         = vec4 1.0 1.0 1.0 1.0

	s_metallic_roughness        = sampler2D texture/T_white
	u_metallic_roughness_uv_set = int 0
	u_metallic_factor           = float 0.5
	u_roughness_factor          = float 0.5
}
@stage: vert {
	#include <common/vert.glsl>
}
@stage: frag {
	#include <common/frag_attrs.glsl>
	#include <common/lighting.glsl>

	#include <common/pbr_frag.glsl>
}
