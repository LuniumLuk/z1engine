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
	#include <common/gbuffer_out_phone.glslh>
#elif defined(VARIANT_SHADOW)
	#include <common/shadow_out.glslh>
#elif defined(VARIANT_VELOCITY)
	#include <common/velocity_out.glslh>
#else
	#include <common/lighting.glslh>

	void main() {
		vec4 color = v_color * texture(s_base_color, v_texcoord0);
		if (u_alpha_mode == 0) {
			color.a = 1.0;
		}
		else if (u_alpha_mode == 1) {
			if (color.a < u_alpha_cutoff) discard;
		}
		frag_color = phone_shading(normalize(v_normal), v_world_position, color, get_shadow());
	}
#endif
}
