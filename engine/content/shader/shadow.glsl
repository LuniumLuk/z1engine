@uniforms: {
	#include <common/uniforms.glslh>
}

@stage: vert {
#define SHADOW
	uniform int u_csm_index;
	#include <common/vert.glslh>
}

@stage: frag {
	// no color output; depth-only pass
	uniform sampler2D u_base_color_texture;

	in vec2 v_texcoord0;

	void main() {
		if (u_alpha_mode == 1) { // Mask
			float alpha = texture(u_base_color_texture, v_texcoord0).a;
			if (alpha < u_alpha_cutoff) {
				discard;
			}
		}
	}
}
