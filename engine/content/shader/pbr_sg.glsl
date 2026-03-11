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
@stage: vert {
	#include <common/vert.glslh>
}
@stage: frag {
	#include <common/frag_attrs.glslh>
	#include <common/lighting.glslh>

#define SPECULAR_GLOSSINESS
	#include <common/pbr_frag.glslh>
}
