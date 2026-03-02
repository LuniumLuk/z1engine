@uniforms: {
	#include <common/uniforms.glsl>
}

@stage: vert {
	// render only depth from light's projection
	#include <common/vert.glsl>
	void main() {
		vec3 world_position = (u_model * vec4(a_position, 1.0)).xyz;
		gl_Position = u_sun_projview * vec4(world_position, 1.0);
	}
}

@stage: frag {
	// no color output; depth-only pass
	void main() {
		// nothing
	}
}
