@uniforms: {
	#include <include/uniforms.glsl>
	uniform sampler2D u_texture;
}

@stage: vert {
	// Per-particle vertex attributes (one vertex per particle)
	// In a billboard expansion approach, VertexID is used to generate quad corners
	layout(location = 0) in vec3 a_position;    // particle world position
	layout(location = 1) in float a_size;       // particle size
	layout(location = 2) in vec4 a_color;       // particle color (RGBA)
	layout(location = 3) in float a_rotation;   // particle rotation in degrees
	layout(location = 4) in vec2 a_texcoord;    // base texcoord (typically 0,0)

	layout(location = 0) out vec4 v_color;
	layout(location = 1) out vec2 v_texcoord;

	void main() {
		// Determine which corner of the quad this vertex is
		// gl_VertexID % 4: 0=bottom-left, 1=bottom-right, 2=top-right, 3=top-left
		int corner = gl_VertexID % 4;

		// Billboard quad coordinates (before rotation) [-1, 1]
		vec2 quad_offset = vec2(0.0);
		vec2 texcoord = vec2(0.0);

		switch (corner) {
			case 0:  // bottom-left
				quad_offset = vec2(-1.0, -1.0);
				texcoord = vec2(0.0, 0.0);
				break;
			case 1:  // bottom-right
				quad_offset = vec2(1.0, -1.0);
				texcoord = vec2(1.0, 0.0);
				break;
			case 2:  // top-right
				quad_offset = vec2(1.0, 1.0);
				texcoord = vec2(1.0, 1.0);
				break;
			case 3:  // top-left
				quad_offset = vec2(-1.0, 1.0);
				texcoord = vec2(0.0, 1.0);
				break;
		}

		// Apply rotation (2D rotation in billboard plane)
		float angle_rad = radians(a_rotation);
		float cos_a = cos(angle_rad);
		float sin_a = sin(angle_rad);

		mat2 rot_matrix = mat2(cos_a, -sin_a, sin_a, cos_a);
		vec2 rotated_offset = rot_matrix * quad_offset;

		// Scale by particle size
		vec2 scaled_offset = rotated_offset * a_size;

		// Billboard: expand quad using camera right/up vectors
		vec3 cam_right = vec3(u_projview[0][0], u_projview[1][0], u_projview[2][0]);
		vec3 cam_up = vec3(u_projview[0][1], u_projview[1][1], u_projview[2][1]);

		// Normalize to handle non-uniform scaling
		cam_right = normalize(cam_right);
		cam_up = normalize(cam_up);

		// World position: center + right * scaled_offset.x + up * scaled_offset.y
		vec3 world_pos = a_position + cam_right * scaled_offset.x + cam_up * scaled_offset.y;

		// Transform to clip space
		gl_Position = u_projview * vec4(world_pos, 1.0);

		// Pass data to fragment shader
		v_color = a_color;
		v_texcoord = texcoord;
	}
}

@stage: frag {
	layout(location = 0) in vec4 v_color;
	layout(location = 1) in vec2 v_texcoord;

	layout(location = 0) out vec4 o_color;

	void main() {
		// Sample texture if available, otherwise use white
		vec4 tex_color = texture(u_texture, v_texcoord);

		// If texture is not bound, tex_color will be (0,0,0,0)
		// In that case, use white as the default texture
		if (tex_color.a < 0.001 && v_texcoord == vec2(0.0)) {
			tex_color = vec4(1.0, 1.0, 1.0, 1.0);
		}

		// Multiply by particle color
		vec4 final_color = tex_color * v_color;

		// Discard fully transparent pixels
		if (final_color.a < 0.001) {
			discard;
		}

		o_color = final_color;
	}
}
