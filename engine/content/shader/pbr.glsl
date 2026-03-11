@uniforms: {
	#include <common/uniforms.glslh>
	#include <common/pbr_uniforms.glslh>
}
@reflections: {
	#include <common/reflections.glslh>
	#include <common/pbr_reflections.glslh>

	s_base_color                = sampler2D texture/T_white
	u_base_color_uv_set         = int 0
	u_base_color_factor         = vec4 1.0 1.0 1.0 1.0

	s_metallic_roughness        = sampler2D texture/T_white
	u_metallic_roughness_uv_set = int 0
	u_metallic_factor           = float 0.5
	u_roughness_factor          = float 0.5
}
@stage: vert {
	#include <common/vert.glslh>
}
@stage: frag {
	#include <common/frag_attrs.glslh>
	#include <common/lighting.glslh>

	#include <common/pbr_frag.glslh>
}
