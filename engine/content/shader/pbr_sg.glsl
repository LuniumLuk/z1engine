@uniforms: {
	#include <common/uniforms.glslh>
#define SPECULAR_GLOSSINESS
	#include <common/pbr_uniforms.glslh>
}
@reflections: {
	#include <common/reflections.glslh>
	#include <common/pbr_reflections.glslh>

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
	#include <common/vert.glslh>
}
@stage: frag {
	#include <common/frag_attrs.glslh>

#ifdef VARIANT_GBUFFER
	#include <common/lighting.glslh>
	#include <common/gbuffer_out_sg.glslh>
#elif defined(VARIANT_SHADOW)
	#include <common/shadow_out_sg.glslh>
#elif defined(VARIANT_VELOCITY)
	#include <common/velocity_out.glslh>
#else
	#include <common/lighting.glslh>
#define SPECULAR_GLOSSINESS
	#include <common/pbr_frag.glslh>
#endif
}
