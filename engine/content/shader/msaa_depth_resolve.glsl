@uniforms: {
	uniform sampler2DMS u_depth_ms;
	uniform int u_num_samples; // gl_NumSamples reflects the (single-sample) draw target instead
}
@stage: vert {
	#include <include/quad.glsl>
}
@stage: frag {
	// Resolves a multisampled depth attachment into a single-sample depth target. Depth/stencil
	// glBlitFramebuffer requires equal sample counts, so the resolve is a depth-writing draw.
	// The nearest (minimum) sample is kept: partially covered pixels resolve to the surface
	// closest to the camera for downstream depth tests and screen-space effects.
	void main() {
		ivec2 coord = ivec2(gl_FragCoord.xy);
		float depth = texelFetch(u_depth_ms, coord, 0).r;
		for (int s = 1; s < u_num_samples; ++s) {
			depth = min(depth, texelFetch(u_depth_ms, coord, s).r);
		}
		gl_FragDepth = depth;
	}
}
