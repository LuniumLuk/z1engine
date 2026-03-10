#include "pch.h"
#include "scene/postprocess_system.h"
#include "scene/component/postprocess_volume.h"
#include "scene/component/camera.h"
#include "render/global.h"

namespace z1 {

	void PostProcessSystem::update(Scene* scene) {
		auto camera_entity = scene->get_main_camera();
		if (!camera_entity) return;

		auto& global = *g_runtime_context.m_global;
		glm::vec3 camera_pos = global.cam_position;

		struct PostProcessState {
			float     pp_exposure;
			float     pp_gamma;
			glm::vec4 pp_tint;
			bool      pp_bloom_enabled;
			float     pp_bloom_threshold;
			float     pp_bloom_intensity;
			float     pp_bloom_knee;
		};

		PostProcessState current_settings;
		current_settings.pp_exposure = global.pp_exposure;
		current_settings.pp_gamma = global.pp_gamma;
		current_settings.pp_tint = global.pp_tint;
		current_settings.pp_bloom_enabled = global.pp_bloom_enabled;
		current_settings.pp_bloom_threshold = global.pp_bloom_threshold;
		current_settings.pp_bloom_intensity = global.pp_bloom_intensity;
		current_settings.pp_bloom_knee = global.pp_bloom_knee;

		// 2. Find volumes and blend
		auto view = scene->m_registry.view<TransformComponent, PostprocessVolumeComponent>();

		struct ActiveVolume {
			PostprocessVolumeComponent* component;
			TransformComponent* transform;
			float weight;
		};
		std::vector<ActiveVolume> volumes;

		for (auto [entity, transform, pp] : view.each()) {
			if (!pp.enabled) continue;

			// Calculate weight
			float weight = 0.0f;
			if (pp.is_global) {
				weight = 1.0f;
			}
			else {
				// Local space calculation for box
				glm::mat4 inv_transform = glm::inverse(transform.get_world_transform());
				glm::vec3 local_pos = glm::vec3(inv_transform * glm::vec4(camera_pos, 1.0f));

				glm::vec3 half_extents = glm::vec3(0.5f);
				glm::vec3 dist_vec = glm::abs(local_pos) - half_extents;
				float max_dist = glm::max(dist_vec.x, glm::max(dist_vec.y, dist_vec.z));

				if (max_dist <= 0.0f) {
					// Inside
					weight = 1.0f;
				}
				else if (pp.blend_distance > 0.0f && max_dist < pp.blend_distance) {
					// Interpolate
					weight = 1.0f - (max_dist / pp.blend_distance);
				}
			}

			if (weight > 0.0f) {
				volumes.push_back({ &pp, &transform, weight });
			}
		}

		// Sort by priority
		std::sort(volumes.begin(), volumes.end(), [](const ActiveVolume& a, const ActiveVolume& b) {
			return a.component->priority < b.component->priority;
		});

		// 3. Apply blending to local struct
		for (const auto& vol : volumes) {
			const auto& pp = *vol.component;
			float w = vol.weight;

			if (pp.override_exposure) current_settings.pp_exposure = glm::mix(current_settings.pp_exposure, pp.exposure, w);
			if (pp.override_gamma) current_settings.pp_gamma = glm::mix(current_settings.pp_gamma, pp.gamma, w);
			if (pp.override_tint) current_settings.pp_tint = glm::mix(current_settings.pp_tint, pp.tint, w);

			if (pp.override_bloom_enabled && w > 0.5f) current_settings.pp_bloom_enabled = pp.bloom_enabled;

			if (pp.override_bloom_threshold) current_settings.pp_bloom_threshold = glm::mix(current_settings.pp_bloom_threshold, pp.bloom_threshold, w);
			if (pp.override_bloom_intensity) current_settings.pp_bloom_intensity = glm::mix(current_settings.pp_bloom_intensity, pp.bloom_intensity, w);
			if (pp.override_bloom_knee) current_settings.pp_bloom_knee = glm::mix(current_settings.pp_bloom_knee, pp.bloom_knee, w);
		}

		// Apply override
		global.set_override_postprocess(
			current_settings.pp_exposure,
			current_settings.pp_gamma,
			current_settings.pp_tint,
			current_settings.pp_bloom_enabled,
			current_settings.pp_bloom_threshold,
			current_settings.pp_bloom_intensity,
			current_settings.pp_bloom_knee
		);
	}

}
