#include "pch.h"
#include "core/reflection.h"
#include "scene/entity.h"

// Include all component headers for the hooks
#include "scene/component/base.h"
#include "scene/component/camera.h"
#include "scene/component/mesh.h"
#include "scene/component/light.h"
#include "scene/component/sprite.h"
#include "scene/component/sky_light.h"
#include "scene/component/animation.h"
#include "scene/component/particle.h"
#include "scene/component/postprocess_volume.h"
#include "scene/component/collider.h"
#include "scene/component/physics.h"

namespace z1 {

	// Register component hooks for ECS component types.
	// Must be in a .cpp where Entity is fully defined.
	//
	// Note: StaticMeshComponent, SkeletalMeshComponent, and ScriptComponent
	// do not have default constructors, so they are registered with a
	// manual construct hook instead of the REGISTER_COMPONENT_HOOKS macro.

	REGISTER_COMPONENT_HOOKS(TagComponent)
	REGISTER_COMPONENT_HOOKS(TransformComponent)
	REGISTER_COMPONENT_HOOKS(CameraComponent)
	REGISTER_COMPONENT_HOOKS(LightComponent)
	REGISTER_COMPONENT_HOOKS(SpriteComponent)
	REGISTER_COMPONENT_HOOKS(SkyLightComponent)
	REGISTER_COMPONENT_HOOKS(AnimationComponent)
	REGISTER_COMPONENT_HOOKS(ParticleComponent)
	REGISTER_COMPONENT_HOOKS(PostprocessVolumeComponent)
	REGISTER_COMPONENT_HOOKS(ColliderComponent)
	REGISTER_COMPONENT_HOOKS(PhysicsComponent)

	// StaticMeshComponent: no default ctor (requires shared_ptr<StaticMesh>)
	struct _REFLECT_HOOK_REGISTER_StaticMeshComponent {
		_REFLECT_HOOK_REGISTER_StaticMeshComponent() {
			auto* info = const_cast<TypeInfo*>(TypeRegistry::instance().get("StaticMeshComponent"));
			if (info) {
				info->add_to = [](Entity& entity) { entity.add_component<StaticMeshComponent>(nullptr); };
				info->remove_from = [](Entity& entity) { entity.remove_component<StaticMeshComponent>(); };
				info->has_in = [](Entity const& entity) -> bool { return entity.has_component<StaticMeshComponent>(); };
			}
		}
	};
	static _REFLECT_HOOK_REGISTER_StaticMeshComponent _REFLECT_HOOK_INSTANCE_StaticMeshComponent;

	// SkeletalMeshComponent: no default ctor (requires shared_ptr<SkeletalMesh>)
	struct _REFLECT_HOOK_REGISTER_SkeletalMeshComponent {
		_REFLECT_HOOK_REGISTER_SkeletalMeshComponent() {
			auto* info = const_cast<TypeInfo*>(TypeRegistry::instance().get("SkeletalMeshComponent"));
			if (info) {
				info->add_to = [](Entity& entity) { entity.add_component<SkeletalMeshComponent>(nullptr); };
				info->remove_from = [](Entity& entity) { entity.remove_component<SkeletalMeshComponent>(); };
				info->has_in = [](Entity const& entity) -> bool { return entity.has_component<SkeletalMeshComponent>(); };
			}
		}
	};
	static _REFLECT_HOOK_REGISTER_SkeletalMeshComponent _REFLECT_HOOK_INSTANCE_SkeletalMeshComponent;

	// ScriptComponent: no default ctor (requires weak_ptr<Entity>)
	struct _REFLECT_HOOK_REGISTER_ScriptComponent {
		_REFLECT_HOOK_REGISTER_ScriptComponent() {
			auto* info = const_cast<TypeInfo*>(TypeRegistry::instance().get("ScriptComponent"));
			if (info) {
				info->add_to = [](Entity& entity) {
					entity.add_component<ScriptComponent>(entity.get_weak_ptr());
				};
				info->remove_from = [](Entity& entity) {
					entity.remove_component<ScriptComponent>();
				};
				info->has_in = [](Entity const& entity) -> bool {
					return entity.has_component<ScriptComponent>();
				};
			}
		}
	};
	static _REFLECT_HOOK_REGISTER_ScriptComponent _REFLECT_HOOK_INSTANCE_ScriptComponent;

	void ForceLinkReflectionHooks() {}

}
