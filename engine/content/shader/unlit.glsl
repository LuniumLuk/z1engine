@uniforms: {
	#include <common/uniforms.glslh>

	uniform sampler2D s_base_color;
}
@reflections: {
	#include <common/reflections.glslh>

	s_base_color = sampler2D texture/T_white
}
@stage: vert {
	#include <common/vert.glslh>
}
@stage: frag {
	#include <common/frag_attrs.glslh>

	void main() {
		frag_color = v_color * texture(s_base_color, v_texcoord0);
	}
}
