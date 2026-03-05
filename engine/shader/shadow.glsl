@uniforms: {
	#include <common/uniforms.glsl>
}

@stage: vert {
#define SHADOW
	#include <common/vert.glsl>
}

@stage: frag {
	// no color output; depth-only pass
	void main() {
		// nothing
	}
}
