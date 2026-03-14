@uniforms: {
	#include <include/uniforms.glsl>
	#include <include/pbr_uniforms.glsl>
}
@reflections: {
	#include <include/reflections.glsl>
	#include <include/pbr_reflections.glsl>

	s_base_color                = sampler2D texture/T_white
	u_base_color_uv_set         = int 0
	u_base_color_factor         = vec4 1.0 1.0 1.0 1.0

	s_metallic_roughness        = sampler2D texture/T_white
	u_metallic_roughness_uv_set = int 0
	u_metallic_factor           = float 0.5
	u_roughness_factor          = float 0.5
}
@variants: {
	VARIANT_GBUFFER
	VARIANT_SHADOW
	VARIANT_VELOCITY
}
@stage: vert {
	#include <include/vert.glsl>
}
@stage: frag {
	#include <include/frag_attrs.glsl>

#if defined(VARIANT_GBUFFER)
	#include <include/lighting.glsl>
	#include <include/gbuffer_out.glsl>
#elif defined(VARIANT_SHADOW)
	#include <include/shadow_out.glsl>
#elif defined(VARIANT_VELOCITY)
	#include <include/velocity_out.glsl>
#else
	#include <include/lighting.glsl>
	#include <include/pbr_frag.glsl>
#endif
}
