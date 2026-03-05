#include "pch.h"
#include "scene/animation_system.h"
#include "scene/component/mesh.h"
#include "scene/component/animation.h"
#include "scene/component/base.h"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtc/quaternion.hpp"

namespace z1 {

	static glm::vec3 interpolate_position(std::vector<PositionKeyframe> const& keys, float time) {
		if (keys.empty()) return glm::vec3(0.0f);
		if (keys.size() == 1) return keys[0].value;

		size_t idx = 0;
		for (size_t i = 0; i < keys.size() - 1; ++i) {
			if (time < keys[i + 1].time) {
				idx = i;
				break;
			}
		}

		auto const& k0 = keys[idx];
		auto const& k1 = keys[idx + 1];
		float t = (time - k0.time) / (k1.time - k0.time);
		return glm::mix(k0.value, k1.value, t);
	}

	static glm::quat interpolate_rotation(std::vector<RotationKeyframe> const& keys, float time) {
		if (keys.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		if (keys.size() == 1) return keys[0].value;

		size_t idx = 0;
		for (size_t i = 0; i < keys.size() - 1; ++i) {
			if (time < keys[i + 1].time) {
				idx = i;
				break;
			}
		}

		auto const& k0 = keys[idx];
		auto const& k1 = keys[idx + 1];
		float t = (time - k0.time) / (k1.time - k0.time);
		return glm::slerp(k0.value, k1.value, t);
	}

	static glm::vec3 interpolate_scale(std::vector<ScaleKeyframe> const& keys, float time) {
		if (keys.empty()) return glm::vec3(1.0f);
		if (keys.size() == 1) return keys[0].value;

		size_t idx = 0;
		for (size_t i = 0; i < keys.size() - 1; ++i) {
			if (time < keys[i + 1].time) {
				idx = i;
				break;
			}
		}

		auto const& k0 = keys[idx];
		auto const& k1 = keys[idx + 1];
		float t = (time - k0.time) / (k1.time - k0.time);
		return glm::mix(k0.value, k1.value, t);
	}

	void AnimationSystem::update(Scene* scene, float dt) {
		PROFILE_FUNCTION();

		auto view = scene->m_registry.view<AnimationComponent, SkeletalMeshComponent>();

		for (auto entity : view) {
			auto& anim_comp = view.get<AnimationComponent>(entity);
			auto& mesh_comp = view.get<SkeletalMeshComponent>(entity);

			if (!anim_comp.playing || !anim_comp.animation_asset || !mesh_comp.m_skeleton) continue;

			auto& anim = *anim_comp.animation_asset;
			auto& skeleton = *mesh_comp.m_skeleton;

			anim_comp.current_time += dt * anim.ticks_per_second * anim_comp.speed;

			if (anim_comp.loop) {
				if (anim.duration > 0.0f) {
					anim_comp.current_time = fmod(anim_comp.current_time, anim.duration);
					if (anim_comp.current_time < 0.0f) anim_comp.current_time += anim.duration;
				}
			}
			else {
				anim_comp.current_time = glm::clamp(anim_comp.current_time, 0.0f, anim.duration);
			}

			std::vector<glm::mat4> local_transforms(skeleton.bones.size());

			// Initialize with bind pose
			for (size_t i = 0; i < skeleton.bones.size(); ++i) {
				local_transforms[i] = skeleton.bones[i].local_bind_transform;
			}

			// Apply animation channels
			for (auto const& channel : anim.channels) {
				int bone_idx = -1;
				// Try match by ID first (fast)
				// Assuming channel.bone_id corresponds to skeleton.bones[].id
				// But we need bone index in array.
				// Since we saved ID == index in import_skeleton, we can use it directly?
				// Usually IDs are consecutive 0..N-1 if we created them that way.
				if (channel.bone_id >= 0 && channel.bone_id < (int)skeleton.bones.size()) {
					if (skeleton.bones[channel.bone_id].id == channel.bone_id) {
						bone_idx = channel.bone_id;
					}
				}

				// Fallback to search
				if (bone_idx == -1) {
					for(size_t i=0; i<skeleton.bones.size(); ++i) {
						 if (skeleton.bones[i].id == channel.bone_id) {
							 bone_idx = (int)i;
							 break;
						 }
					}
				}

				if (bone_idx == -1) continue;

				glm::vec3 pos = interpolate_position(channel.position_keys, anim_comp.current_time);
				glm::quat rot = interpolate_rotation(channel.rotation_keys, anim_comp.current_time);
				glm::vec3 scl = interpolate_scale(channel.scale_keys, anim_comp.current_time);

				local_transforms[bone_idx] = glm::translate(glm::mat4(1.0f), pos) * glm::mat4(rot) * glm::scale(glm::mat4(1.0f), scl);
			}

			// Compute global transforms
			// Assuming parent comes before child in array (topological sort)
			// If not, we need recursive approach or re-order.
			// glTF usually exports in hierarchy order (DFS).

			std::vector<glm::mat4> global_transforms(skeleton.bones.size());

			for (size_t i = 0; i < skeleton.bones.size(); ++i) {
				int parent = skeleton.bones[i].parent_id;
				if (parent != -1 && parent < (int)i) {
					global_transforms[i] = global_transforms[parent] * local_transforms[i];
				} else if (parent != -1) {
					// Parent is after child? This case requires recursion or multi-pass.
					// For now assume sorted.
					// If parent is not processed, we use identity?
					// Or just use local?
					global_transforms[i] = local_transforms[i];
				} else {
					global_transforms[i] = local_transforms[i];
				}
			}

			// Final skinning matrices = GlobalTransform * InverseBindPose
			anim_comp.bone_matrices.assign(100, glm::mat4(1.0f));
			for (size_t i = 0; i < skeleton.bones.size(); ++i) {
				if (i >= 100) break;
				anim_comp.bone_matrices[i] = global_transforms[i] * skeleton.bones[i].offset_matrix;
			}
		}
	}

}
