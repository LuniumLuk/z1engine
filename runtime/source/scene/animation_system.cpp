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
		if (time <= keys.front().time) return keys.front().value;
		if (time >= keys.back().time) return keys.back().value;

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
		if (time <= keys.front().time) return keys.front().value;
		if (time >= keys.back().time) return keys.back().value;

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
		if (time <= keys.front().time) return keys.front().value;
		if (time >= keys.back().time) return keys.back().value;

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

				// 1. Try match by Node Index (if available)
				// channel.bone_id is the GLTF Node Index
				for(size_t i=0; i<skeleton.bones.size(); ++i) {
					 if (skeleton.bones[i].node_index != -1 && skeleton.bones[i].node_index == channel.bone_id) {
						 bone_idx = (int)i;
						 break;
					 }
				}

				// 2. Fallback: Match by Name (slower but robust)
				if (bone_idx == -1) {
					for(size_t i=0; i<skeleton.bones.size(); ++i) {
						 if (skeleton.bones[i].name == channel.bone_name) {
							 bone_idx = (int)i;
							 break;
						 }
					}
				}

				if (bone_idx == -1) continue;

				glm::vec3 pos(0.0f);
				glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);
				glm::vec3 scl(1.0f);
				glm::vec3 skew(0.0f);
				glm::vec4 perspective(0.0f, 0.0f, 0.0f, 1.0f);
				glm::decompose(local_transforms[bone_idx], scl, rot, pos, skew, perspective);

				if (!channel.position_keys.empty())
					pos = interpolate_position(channel.position_keys, anim_comp.current_time);
				if (!channel.rotation_keys.empty())
					rot = interpolate_rotation(channel.rotation_keys, anim_comp.current_time);
				if (!channel.scale_keys.empty())
					scl = interpolate_scale(channel.scale_keys, anim_comp.current_time);

				local_transforms[bone_idx] = glm::translate(glm::mat4(1.0f), pos) * glm::mat4(rot) * glm::scale(glm::mat4(1.0f), scl);
			}

			// Compute global transforms
			std::vector<glm::mat4> global_transforms(skeleton.bones.size());
			std::vector<bool> computed(skeleton.bones.size(), false);

			std::function<glm::mat4 const&(int)> get_global_transform =
				[&](int bone_idx) -> glm::mat4 const& {
				if (computed[bone_idx]) return global_transforms[bone_idx];

				int parent_idx = skeleton.bones[bone_idx].parent_id;
				if (parent_idx != -1) {
					global_transforms[bone_idx] = get_global_transform(parent_idx) * local_transforms[bone_idx];
				} else {
					global_transforms[bone_idx] = local_transforms[bone_idx];
				}

				computed[bone_idx] = true;
				return global_transforms[bone_idx];
			};

			for (size_t i = 0; i < skeleton.bones.size(); ++i) {
				get_global_transform((int)i);
			}

			// Final skinning matrices = GlobalTransform * InverseBindPose
			anim_comp.bone_matrices.assign(MAX_BONES, glm::mat4(1.0f));
			for (size_t i = 0; i < skeleton.bones.size(); ++i) {
				if (i >= MAX_BONES)
					break;
				anim_comp.bone_matrices[i] = global_transforms[i] * skeleton.bones[i].offset_matrix;
			}

			// Upload to uniform buffer
			if (!anim_comp.bone_ubo) {
				anim_comp.bone_ubo = UniformBuffer::create(nullptr, MAX_BONES * sizeof(glm::mat4), BufferUsage::Dynamic);
			}
			anim_comp.bone_ubo->write(anim_comp.bone_matrices.data(), skeleton.bones.size() * sizeof(glm::mat4));
		}
	}

}
