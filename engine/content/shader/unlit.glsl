@uniforms: {
	#include <common/uniforms.glslh>

	uniform sampler2D s_base_color;
}
@reflections: {
	#include <common/reflections.glslh>

	s_base_color = sampler2D texture/T_white
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
	#include <common/gbuffer_out_unlit.glslh>
#elif defined(VARIANT_SHADOW)
	#include <common/shadow_out.glslh>
#elif defined(VARIANT_VELOCITY)
	#include <common/velocity_out.glslh>
#else
	void main() {
		frag_color = v_color * texture(s_base_color, v_texcoord0);
	}
#endif
}
