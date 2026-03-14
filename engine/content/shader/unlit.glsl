@uniforms: {
	#include <include/uniforms.glsl>

	uniform sampler2D s_base_color;
}
@reflections: {
	#include <include/reflections.glsl>

	s_base_color = sampler2D texture/T_white
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
	#include <include/gbuffer_out_unlit.glsl>
#elif defined(VARIANT_SHADOW)
	#include <include/shadow_out.glsl>
#elif defined(VARIANT_VELOCITY)
	#include <include/velocity_out.glsl>
#else
	void main() {
		frag_color = v_color * texture(s_base_color, v_texcoord0);
	}
#endif
}
