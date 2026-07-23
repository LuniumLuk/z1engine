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
	uniform int u_receive_shadows;
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
	layout(location = 2) out vec3 v_world_pos;

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
		v_world_pos = world_pos;
	}
}

@stage: frag {
	layout(location = 0) in vec4 v_color;
	layout(location = 1) in vec2 v_texcoord;
	layout(location = 2) in vec3 v_world_pos;

	layout(location = 0) out vec4 o_color;

	// Linearize a [0,1] depth buffer value to view-space distance
	float linearize_depth(float d) {
		return u_near * u_far / (u_far - d * (u_far - u_near));
	}

	// 3x3 PCF with 4-tap bilinear per sample
	float get_particle_shadow(vec3 world_pos) {
		float dist = distance(u_cam_position.xyz, world_pos);
		int layer = 0;
		if (dist >= u_csm_splits.x) layer = 1;
		if (dist >= u_csm_splits.y) layer = 2;
		if (dist >= u_csm_splits.z) layer = 3;

		vec4 ls = u_sun_projview[layer] * vec4(world_pos, 1.0);
		vec3 proj = ls.xyz / ls.w;
		vec3 uv = vec3(proj.xy * 0.5 + 0.5, float(layer));
		float current_depth = proj.z * 0.5 + 0.5;

		float bias = 0.002;

		ivec3 tex_size = textureSize(u_shadow_map, 0);
		vec2 texel_size = 1.0 / vec2(tex_size.xy);
		int z = int(uv.z);

		float shadow = 0.0;
		for (int x = -1; x <= 1; ++x) {
			for (int y = -1; y <= 1; ++y) {
				vec2 sample_uv = uv.xy + vec2(float(x), float(y)) * texel_size;

				// 4-tap bilinear PCF at this grid position
				vec2 texel_pos = sample_uv * vec2(tex_size.xy) - 0.5;
				vec2 frac_pos = fract(texel_pos);
				ivec2 base = ivec2(floor(texel_pos));
				ivec2 next = base + ivec2(1, 1);

				float s00 = texelFetch(u_shadow_map, ivec3(base.x, base.y, z), 0).r < current_depth - bias ? 0.0 : 1.0;
				float s10 = texelFetch(u_shadow_map, ivec3(next.x, base.y, z), 0).r < current_depth - bias ? 0.0 : 1.0;
				float s01 = texelFetch(u_shadow_map, ivec3(base.x, next.y, z), 0).r < current_depth - bias ? 0.0 : 1.0;
				float s11 = texelFetch(u_shadow_map, ivec3(next.x, next.y, z), 0).r < current_depth - bias ? 0.0 : 1.0;

				shadow += mix(mix(s00, s10, frac_pos.x), mix(s01, s11, frac_pos.x), frac_pos.y);
			}
		}
		return shadow / 9.0;
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

		// Shadow reception
		if (u_receive_shadows != 0) {
			float shadow = get_particle_shadow(v_world_pos);
			final_color.rgb *= shadow;
		}

		// Discard fully transparent pixels
		if (final_color.a < 0.001) {
			discard;
		}

		o_color = final_color;
	}
}
