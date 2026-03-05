@uniforms: {
	#include <common/uniforms.glsl>

	uniform sampler2D u_input;
}
@stage: vert {
	#include <common/quad.glsl>
}
@stage: frag {
	layout(location = 0) in vec2 v_uv;
	layout(location = 0) out vec4 frag_color;

	void main() {
		frag_color = texture(u_input, v_uv);
	}
}
