@uniforms: {
	#include <common/uniforms.glsl>
	uniform sampler2D u_sky_texture;
	uniform float u_rotation;
	uniform float u_intensity;
	uniform float u_mip_level;
	uniform mat4 u_inv_projview;
}

@stage: vert {
	layout(location = 0) in vec2 a_position;
	layout(location = 0) out vec2 v_uv;

	void main() {
		v_uv = a_position * 0.5 + 0.5;
		// Force depth to 1.0 (far plane)
		// We use 0.999999 to ensure it passes LessEqual test against 1.0
		gl_Position = vec4(a_position, 1.0 - 1e-6, 1.0);
	}
}

@stage: frag {
	layout(location = 0) in vec2 v_uv;
	layout(location = 0) out vec4 o_color;

	const vec2 invAtan = vec2(0.1591, 0.3183);

	vec2 SampleSphericalMap(vec3 v)
	{
		vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
		uv *= invAtan;
		uv += 0.5;
		return uv;
	}

	vec3 RotateY(vec3 v, float angle)
	{
		float s = sin(radians(angle));
		float c = cos(radians(angle));
		return vec3(c * v.x - s * v.z, v.y, s * v.x + c * v.z);
	}

	void main() {
		// Reconstruct view direction from screen position
		vec4 clip_pos = vec4(v_uv * 2.0 - 1.0, 1.0, 1.0);
		vec4 world_pos = u_inv_projview * clip_pos;
		world_pos /= world_pos.w;

		vec3 view_dir = normalize(world_pos.xyz - u_cam_position.xyz);

		// Apply rotation
		vec3 rotated_dir = RotateY(view_dir, -u_rotation);

		vec2 uv = SampleSphericalMap(normalize(rotated_dir));

		vec3 color = textureLod(u_sky_texture, uv, u_mip_level).rgb;
		o_color = vec4(color * u_intensity, 1.0);
	}
}
