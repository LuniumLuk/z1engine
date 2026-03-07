@uniforms: {
	#include <common/uniforms.glsl>
}

@stage: vert {
#define SHADOW
	uniform int u_csm_index;
	#include <common/vert.glsl>
}

@stage: frag {
	// no color output; depth-only pass
	void main() {
		// nothing
	}
}
