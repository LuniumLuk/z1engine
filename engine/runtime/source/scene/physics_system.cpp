#include "pch.h"
#ifdef PLATFORM_WINDOWS
#include "scene/physics_system.h"
#include "scene/scene.h"
#include "scene/component/collider.h"
#include "scene/component/physics.h"
#include "scene/component/base.h"
#include "core/log.h"

#include "PxPhysicsAPI.h"

#include <unordered_map>

namespace z1 {
namespace {

	// File-static PhysX state (singleton)
	bool s_initialized = false;
	physx::PxDefaultAllocator* s_allocator = nullptr;
	physx::PxDefaultErrorCallback* s_error_callback = nullptr;
	physx::PxFoundation* s_foundation = nullptr;
	physx::PxPhysics* s_physics = nullptr;
	physx::PxDefaultCpuDispatcher* s_dispatcher = nullptr;
	physx::PxScene* s_px_scene = nullptr;
	physx::PxMaterial* s_default_material = nullptr;

	constexpr float k_gravity = -9.81f;
	constexpr float k_static_friction = 0.5f;
	constexpr float k_dynamic_friction = 0.5f;
	constexpr float k_restitution = 0.1f;
	constexpr uint32_t k_dispatcher_threads = 1;

	std::unordered_map<entt::entity, physx::PxRigidActor*> s_actors;

	void init_once() {
		if (s_initialized) return;

		s_allocator = new physx::PxDefaultAllocator();
		s_error_callback = new physx::PxDefaultErrorCallback();

		s_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, *s_allocator, *s_error_callback);
		if (!s_foundation) {
			CORE_ERROR("PhysX: failed to create foundation");
			delete s_error_callback; s_error_callback = nullptr;
			delete s_allocator; s_allocator = nullptr;
			return;
		}

		s_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *s_foundation, physx::PxTolerancesScale());
		if (!s_physics) {
			CORE_ERROR("PhysX: failed to create physics");
			s_foundation->release(); s_foundation = nullptr;
			delete s_error_callback; s_error_callback = nullptr;
			delete s_allocator; s_allocator = nullptr;
			return;
		}

		physx::PxSceneDesc scene_desc(s_physics->getTolerancesScale());
		scene_desc.gravity = physx::PxVec3(0.0f, k_gravity, 0.0f);
		scene_desc.cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(k_dispatcher_threads);
		scene_desc.filterShader = physx::PxDefaultSimulationFilterShader;
		s_dispatcher = static_cast<physx::PxDefaultCpuDispatcher*>(scene_desc.cpuDispatcher);

		s_px_scene = s_physics->createScene(scene_desc);
		if (!s_px_scene) {
			CORE_ERROR("PhysX: failed to create scene");
			s_dispatcher->release(); s_dispatcher = nullptr;
			s_physics->release(); s_physics = nullptr;
			s_foundation->release(); s_foundation = nullptr;
			delete s_error_callback; s_error_callback = nullptr;
			delete s_allocator; s_allocator = nullptr;
			return;
		}

		s_default_material = s_physics->createMaterial(k_static_friction, k_dynamic_friction, k_restitution);
		if (!s_default_material) {
			CORE_ERROR("PhysX: failed to create default material");
			s_px_scene->release(); s_px_scene = nullptr;
			s_dispatcher->release(); s_dispatcher = nullptr;
			s_physics->release(); s_physics = nullptr;
			s_foundation->release(); s_foundation = nullptr;
			delete s_error_callback; s_error_callback = nullptr;
			delete s_allocator; s_allocator = nullptr;
			return;
		}

		s_initialized = true;
		CORE_DEBUG("PhysX: initialized");
	}

	physx::PxTransform glm_to_px_transform(glm::vec3 const& pos, glm::vec3 const& rot) {
		glm::quat q(glm::radians(rot));
		return physx::PxTransform(
			physx::PxVec3(pos.x, pos.y, pos.z),
			physx::PxQuat(q.x, q.y, q.z, q.w));
	}

	void px_to_glm_transform(physx::PxTransform const& px, glm::vec3& out_pos, glm::vec3& out_rot) {
		out_pos = glm::vec3(px.p.x, px.p.y, px.p.z);
		glm::quat q(px.q.w, px.q.x, px.q.y, px.q.z);
		out_rot = glm::degrees(glm::eulerAngles(q));
	}

	void create_actor(entt::entity entity, Scene* scene) {
		auto& collider = scene->m_registry.get<ColliderComponent>(entity);
		auto& physics = scene->m_registry.get<PhysicsComponent>(entity);
		auto& transform = scene->m_registry.get<TransformComponent>(entity);

		physx::PxTransform pose = glm_to_px_transform(transform.m_location, transform.m_rotation);

		physx::PxGeometry* geometry = nullptr;
		physx::PxSphereGeometry sphere_geom;
		physx::PxBoxGeometry box_geom;
		physx::PxCapsuleGeometry capsule_geom;

		switch (collider.m_shape) {
		case ColliderShape::Sphere:
			sphere_geom = physx::PxSphereGeometry(collider.m_half_extents.x);
			geometry = &sphere_geom;
			break;
		case ColliderShape::Box:
			box_geom = physx::PxBoxGeometry(
				collider.m_half_extents.x,
				collider.m_half_extents.y,
				collider.m_half_extents.z);
			geometry = &box_geom;
			break;
		case ColliderShape::Capsule:
			capsule_geom = physx::PxCapsuleGeometry(
				collider.m_half_extents.x,
				collider.m_half_extents.y);
			geometry = &capsule_geom;
			break;
		}

		if (!geometry) return;

		physx::PxRigidActor* actor = nullptr;

		switch (physics.m_mode) {
		case PhysicsMode::Static: {
			actor = s_physics->createRigidStatic(pose);
			break;
		}
		case PhysicsMode::Kinematic: {
			physx::PxRigidDynamic* dyn = s_physics->createRigidDynamic(pose);
			dyn->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
			actor = dyn;
			break;
		}
		case PhysicsMode::Dynamic: {
			physx::PxRigidDynamic* dyn = s_physics->createRigidDynamic(pose);
			dyn->setMass(physics.m_mass);
			dyn->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !physics.m_use_gravity);
			dyn->setLinearDamping(physics.m_linear_damping);
			actor = dyn;
			break;
		}
		}

		if (!actor) return;

		physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(
			*actor, *geometry, *s_default_material);
		if (shape) {
			s_px_scene->addActor(*actor);
			s_actors[entity] = actor;
		}
	}

	void cleanup_stale_actors(Scene* scene) {
		std::vector<entt::entity> to_remove;

		for (auto& [entity, actor] : s_actors) {
			if (!scene->m_registry.valid(entity) ||
				!scene->m_registry.all_of<ColliderComponent, PhysicsComponent>(entity)) {
				actor->release();
				to_remove.push_back(entity);
			}
		}

		for (auto& entity : to_remove) {
			s_actors.erase(entity);
		}
	}

} // namespace

	void PhysicsSystem::update(Scene* scene, float dt) {
		init_once();
		if (!s_px_scene) return;

		PROFILE_FUNCTION();

		cleanup_stale_actors(scene);

		auto view = scene->m_registry.view<ColliderComponent, PhysicsComponent, TransformComponent>();
		for (auto entity : view) {
			if (s_actors.find(entity) == s_actors.end()) {
				create_actor(entity, scene);
			}
		}

		for (auto [entity, actor] : s_actors) {
			auto& physics = scene->m_registry.get<PhysicsComponent>(entity);
			if (physics.m_mode == PhysicsMode::Kinematic) {
				auto& transform = scene->m_registry.get<TransformComponent>(entity);
				actor->setGlobalPose(glm_to_px_transform(transform.m_location, transform.m_rotation));
			}
		}

		s_px_scene->simulate(dt);
		s_px_scene->fetchResults(true);

		for (auto [entity, actor] : s_actors) {
			auto& physics = scene->m_registry.get<PhysicsComponent>(entity);
			if (physics.m_mode == PhysicsMode::Dynamic) {
				auto& transform = scene->m_registry.get<TransformComponent>(entity);
				physx::PxTransform pose = actor->getGlobalPose();
				glm::vec3 pos, rot;
				px_to_glm_transform(pose, pos, rot);
				transform.m_location = pos;
				transform.m_rotation = rot;
			}
		}
	}

}

#endif // PLATFORM_WINDOWS
