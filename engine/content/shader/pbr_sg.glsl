@uniforms: {
	#include <include/uniforms.glsl>
#define SPECULAR_GLOSSINESS
	#include <include/pbr_uniforms.glsl>
}
@reflections: {
	#include <include/reflections.glsl>
	#include <include/pbr_reflections.glsl>

	s_diffuse                   = sampler2D texture/T_white
	u_diffuse_factor            = vec4 1.0 1.0 1.0 1.0

	s_specular_glossiness       = sampler2D texture/T_white
	u_specular_factor           = vec3 1.0 1.0 1.0
	u_glossiness_factor         = float 1.0
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

#ifdef VARIANT_GBUFFER
	#include <include/lighting.glsl>
	#include <include/gbuffer_out_sg.glsl>
#elif defined(VARIANT_SHADOW)
	#include <include/shadow_out_sg.glsl>
#elif defined(VARIANT_VELOCITY)
	#include <include/velocity_out.glsl>
#else
	#include <include/lighting.glsl>
#define SPECULAR_GLOSSINESS
	#include <include/pbr_frag.glsl>
#endif
}
