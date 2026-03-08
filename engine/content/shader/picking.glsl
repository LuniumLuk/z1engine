@uniforms: {
	#include <common/uniforms.glsl>

	layout(location = 2) uniform float u_object_id;
}
@stage: vert {
	#include <common/vert.glsl>
}
@stage: frag {
	layout(location = 0) out vec4 frag_color;

	vec4 pack_uint_to_rgba8(uint v) {
		return vec4(
			float((v >>  0) & 0xFFu) / 255.0,
			float((v >>  8) & 0xFFu) / 255.0,
			float((v >> 16) & 0xFFu) / 255.0,
			float((v >> 24) & 0xFFu) / 255.0
		);
	}

	void main() {
		frag_color = pack_uint_to_rgba8(uint(u_object_id));
	}
}
