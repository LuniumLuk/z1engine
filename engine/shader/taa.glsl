@uniforms: {
	#include <common/uniforms.glsl>

	uniform sampler2D u_current_color;
	uniform sampler2D u_history_color;
	uniform sampler2D u_velocity;
}
@stage: vert {
	#include <common/quad.glsl>
}
@stage: frag {
	layout(location = 0) in vec2 v_uv;
	layout(location = 0) out vec4 frag_color;

	// Utility functions for YCoCg
	vec3 RGBToYCoCg(vec3 rgb) {
		float Y = dot(rgb, vec3(0.25, 0.50, 0.25));
		float Co = dot(rgb, vec3(0.50, 0.00, -0.50));
		float Cg = dot(rgb, vec3(-0.25, 0.50, -0.25));
		return vec3(Y, Co, Cg);
	}

	vec3 YCoCgToRGB(vec3 ycocg) {
		float Y = ycocg.x;
		float Co = ycocg.y;
		float Cg = ycocg.z;
		float R = Y + Co - Cg;
		float G = Y + Cg;
		float B = Y - Co - Cg;
		return vec3(R, G, B);
	}

	vec3 ClipAABB(vec3 aabbMin, vec3 aabbMax, vec3 prevSample) {
		vec3 p_clip = 0.5 * (aabbMax + aabbMin);
		vec3 e_clip = 0.5 * (aabbMax - aabbMin);

		vec3 v_clip = prevSample - p_clip;
		vec3 v_unit = v_clip.xyz / e_clip.xyz;
		vec3 a_unit = abs(v_unit);
		float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

		if (ma_unit > 1.0) {
			return p_clip + v_clip / ma_unit;
		} else {
			return prevSample;
		}
	}

	void main() {
		// Read motion vector (in UV space)
		vec4 velocity_sample = texture(u_velocity, v_uv);
		vec2 velocity = velocity_sample.xy;
		float velocity_mask = velocity_sample.a; // 0.0 if background, 1.0 if object

		// Reproject UV into previous frame
		vec2 prev_uv = v_uv - velocity;

		// Get resolution
		ivec2 texSize = textureSize(u_current_color, 0);
		vec2 texelSize = 1.0 / vec2(texSize);

		// Sample 3x3 neighborhood to find min/max
		vec3 minColor = vec3(10000.0);
		vec3 maxColor = vec3(-10000.0);

		vec3 current = texture(u_current_color, v_uv).rgb;

		// Use a 3x3 kernel
		for(int x = -1; x <= 1; ++x) {
			for(int y = -1; y <= 1; ++y) {
				vec3 c = texture(u_current_color, v_uv + vec2(x, y) * texelSize).rgb;
				vec3 c_ycocg = RGBToYCoCg(c);
				minColor = min(minColor, c_ycocg);
				maxColor = max(maxColor, c_ycocg);
			}
		}

		if (u_taa_enabled < 0.5) {
			frag_color = vec4(current, 1.0);
			return;
		}

		// Sample history
		vec3 history = texture(u_history_color, prev_uv).rgb;
		vec3 history_ycocg = RGBToYCoCg(history);

		// Clip history to AABB of current neighborhood
		history_ycocg = ClipAABB(minColor, maxColor, history_ycocg);

		// Mix
		// Adjust blend factor based on motion
		// If velocity is high, trust history less (to avoid trails)
		// But here we just use the provided blend factor
		float blend = u_taa_blend;

		// If background (no velocity written), don't blend history
		blend *= velocity_mask;

		// Feedback
		vec3 result_ycocg = mix(RGBToYCoCg(current), history_ycocg, blend);
		vec3 result = YCoCgToRGB(result_ycocg);

		// Prevent NaNs/Infs
		result = max(vec3(0.0), result);

		frag_color = vec4(result, 1.0);
	}
}
