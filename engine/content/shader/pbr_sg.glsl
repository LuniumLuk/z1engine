@uniforms: {
	#include <common/uniforms.glsl>
#define SPECULAR_GLOSSINESS
	#include <common/pbr_uniforms.glsl>
}
@reflections: {
	#include <common/reflections.glsl>
	#include <common/pbr_reflections.glsl>

	s_diffuse                   = sampler2D texture/T_white
	u_diffuse_factor            = vec4 1.0 1.0 1.0 1.0

	s_specular_glossiness       = sampler2D texture/T_white
	u_specular_factor           = vec3 1.0 1.0 1.0
	u_glossiness_factor         = float 1.0
}
@stage: vert {
	#include <common/vert.glsl>
}
@stage: frag {
	#include <common/frag_attrs.glsl>
	#include <common/lighting.glsl>

#define SPECULAR_GLOSSINESS
	#include <common/pbr_frag.glsl>
}
