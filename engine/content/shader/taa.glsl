@uniforms: {
	#include <include/uniforms.glsl>

	uniform sampler2D u_current_color;
	uniform sampler2D u_history_color;
	uniform sampler2D u_velocity;
}
@stage: vert {
	#include <include/quad.glsl>
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

	// Variance-guided AABB clip (UE4-style)
	// Expands the clip box by gamma * sigma to avoid over-clamping valid detail
	vec3 ClipAABBVar(vec3 aabbMin, vec3 aabbMax, vec3 prevSample, vec3 variance, float clipGamma) {
		vec3 sigma = sqrt(max(vec3(0.0), variance));
		vec3 clipMin = aabbMin - clipGamma * sigma;
		vec3 clipMax = aabbMax + clipGamma * sigma;

		vec3 p_clip = 0.5 * (clipMax + clipMin);
		vec3 e_clip = 0.5 * (clipMax - clipMin) + vec3(1e-6);

		vec3 v_clip = prevSample - p_clip;
		vec3 v_unit = v_clip / e_clip;
		vec3 a_unit = abs(v_unit);
		float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

		if (ma_unit > 1.0) {
			return p_clip + v_clip / ma_unit;
		}
		else {
			return prevSample;
		}
	}

	void main() {
		// Read motion vector (in UV space)
		vec4 velocity_sample = texture(u_velocity, v_uv);
		vec2 velocity = velocity_sample.xy;

		// Get resolution
		ivec2 texSize = textureSize(u_current_color, 0);
		vec2 texelSize = 1.0 / vec2(texSize);

		// Compensate jitter
		vec2 comp_uv = vec2(u_taa_jitter_u, -u_taa_jitter_v);
		vec2 sample_uv = v_uv - comp_uv;
		vec3 current = texture(u_current_color, sample_uv).rgb;

		if (u_taa_enabled < 0.5) {
			frag_color = vec4(current, 1.0);
			return;
		}

		// Reproject UV into previous frame
		// Velocity = curr_uv - prev_uv, so prev_uv = curr_uv - velocity
		vec2 prev_uv = v_uv - velocity;

		// --- 5x5 neighborhood analysis in YCoCg ---
		// Compute mean, variance, and AABB for the neighborhood
		vec3 m1 = vec3(0.0);   // first moment (mean)
		vec3 m2 = vec3(0.0);   // second moment
		vec3 minColor = vec3(10000.0);
		vec3 maxColor = vec3(-10000.0);

		float sampleCount = 0.0;
		for (int x = -2; x <= 2; ++x) {
			for (int y = -2; y <= 2; ++y) {
				vec3 c = texture(u_current_color, sample_uv + vec2(x, y) * texelSize).rgb;
				vec3 c_ycocg = RGBToYCoCg(c);
				m1 += c_ycocg;
				m2 += c_ycocg * c_ycocg;
				minColor = min(minColor, c_ycocg);
				maxColor = max(maxColor, c_ycocg);
				sampleCount += 1.0;
			}
		}
		vec3 mean = m1 / sampleCount;
		vec3 variance = (m2 / sampleCount) - (mean * mean);
		variance = max(vec3(0.0), variance);

		// --- Adaptive blend factor ---
		// blend = new_frame_weight. High variance -> more new frame (less ghosting).
		// Low variance -> less new frame (better accumulation on flat surfaces).
		float lumVariance = dot(variance, vec3(1.0, 0.25, 0.25)); // Y dominates
		float blend = u_taa_blend * (1.0 + u_taa_variance_scale * lumVariance * 100.0);

		// Edge-stop: if reprojected history is very different from local mean,
		// boost new-frame weight (likely disocclusion)
		vec3 history = texture(u_history_color, prev_uv).rgb;
		vec3 history_ycocg = RGBToYCoCg(history);
		vec3 histDiff = abs(history_ycocg - mean) / (sqrt(variance) + vec3(1e-4));
		float histOutlier = max(histDiff.x, max(histDiff.y, histDiff.z));
		float edgeStop = 1.0 + histOutlier * histOutlier; // >1 boosts blend for outliers

		blend *= edgeStop;

		// Clamp blend to sensible range
		blend = clamp(blend, 0.03, 0.3);

		// Reduce history contribution near screen edges (invalid reprojection)
		vec2 edgeBlend = smoothstep(0.0, 0.05, prev_uv) * (1.0 - smoothstep(0.95, 1.0, prev_uv));
		blend = mix(1.0, blend, edgeBlend.x * edgeBlend.y);

		// --- Variance-guided clip ---
		history_ycocg = ClipAABBVar(minColor, maxColor, history_ycocg, variance, u_taa_clip_gamma);

		// --- Blend ---
		vec3 current_ycocg = RGBToYCoCg(current);
		vec3 result_ycocg = mix(history_ycocg, current_ycocg, blend);
		vec3 result = YCoCgToRGB(result_ycocg);

		// Prevent NaNs/Infs
		result = max(vec3(0.0), result);

		frag_color = vec4(result, 1.0);
	}
}
