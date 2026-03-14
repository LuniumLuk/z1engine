@uniforms: {
	#include <include/uniforms.glsl>
	uniform sampler2D u_texture;
	uniform int u_has_texture;
	uniform vec3 u_cam_right;
	uniform vec3 u_cam_up;
	uniform sampler2D u_depth_texture;
	uniform int u_soft_blend;
	uniform float u_near;
	uniform float u_far;
}

@stage: vert {
	// Per-vertex quad attributes (from shared unit quad VBO)
	layout(location = 0) in vec2 a_quad_offset;  // billboard offset [-1,1]
	layout(location = 1) in vec2 a_texcoord;      // UV [0,1]

	// Per-instance attributes (from instance buffer, one per particle)
	layout(location = 2) in vec3 a_position;      // particle world position
	layout(location = 3) in float a_size;         // particle size
	layout(location = 4) in vec4 a_color;         // particle color (RGBA)
	layout(location = 5) in float a_rotation;     // particle rotation in degrees

	layout(location = 0) out vec4 v_color;
	layout(location = 1) out vec2 v_texcoord;

	void main() {
		// Apply rotation (2D rotation in billboard plane)
		float angle_rad = radians(a_rotation);
		float cos_a = cos(angle_rad);
		float sin_a = sin(angle_rad);

		mat2 rot_matrix = mat2(cos_a, -sin_a, sin_a, cos_a);
		vec2 rotated_offset = rot_matrix * a_quad_offset;

		// Scale by particle size
		vec2 scaled_offset = rotated_offset * a_size;

		// Billboard: expand quad using explicit camera vectors
		vec3 world_pos = a_position + u_cam_right * scaled_offset.x + u_cam_up * scaled_offset.y;

		// Transform to clip space
		gl_Position = u_projview * vec4(world_pos, 1.0);

		// Pass data to fragment shader
		v_color = a_color;
		v_texcoord = a_texcoord;
	}
}

@stage: frag {
	layout(location = 0) in vec4 v_color;
	layout(location = 1) in vec2 v_texcoord;

	layout(location = 0) out vec4 o_color;

	// Linearize a [0,1] depth buffer value to view-space distance
	float linearize_depth(float d) {
		return u_near * u_far / (u_far - d * (u_far - u_near));
	}

	void main() {
		vec4 tex_color;
		if (u_has_texture != 0) {
			tex_color = texture(u_texture, v_texcoord);
		} else {
			tex_color = vec4(1.0, 1.0, 1.0, 1.0);
		}

		// Multiply by particle color
		vec4 final_color = tex_color * v_color;

		// Soft particle depth fade
		if (u_soft_blend != 0) {
			vec2 screen_uv = gl_FragCoord.xy / vec2(textureSize(u_depth_texture, 0));
			float scene_depth = texture(u_depth_texture, screen_uv).r;
			float frag_depth = gl_FragCoord.z;

			float scene_linear = linearize_depth(scene_depth);
			float frag_linear = linearize_depth(frag_depth);

			// Fade range: proportional to distance so nearby particles fade less aggressively
			float soft_range = max(frag_linear * 0.05, 0.5);
			float fade = clamp((scene_linear - frag_linear) / soft_range, 0.0, 1.0);
			final_color.a *= fade;
		}

		// Discard fully transparent pixels
		if (final_color.a < 0.001) {
			discard;
		}

		o_color = final_color;
	}
}
