// RETIRED: This standalone shadow shader is no longer used.
// Shadow rendering now uses per-material shader variants via
// per_frame.variant_key = ShaderVariant::Shadow, which injects #define VARIANT_SHADOW.
// Each surface shader includes include/shadow_out.glsl for alpha mask support.
// Kept as reference only.

@uniforms: {
	#include <include/uniforms.glsl>
}

@stage: vert {
#define SHADOW
	uniform int u_csm_index;
	#include <include/vert.glsl>
}

@stage: frag {
	// no color output; depth-only pass
	uniform sampler2D s_alpha;
	uniform int u_alpha_uv_set;

	layout(location = 2) in vec2 v_texcoord0;
	layout(location = 3) in vec2 v_texcoord1;

	vec2 get_uv(int uv_set) {
		if (uv_set == 1) return v_texcoord1;
		return v_texcoord0;
	}

	void main() {
		if (u_alpha_mode == 1) { // Mask
			float alpha = texture(s_alpha, get_uv(u_alpha_uv_set)).a;
			if (alpha < u_alpha_cutoff) {
				discard;
			}
		}
	}
}
