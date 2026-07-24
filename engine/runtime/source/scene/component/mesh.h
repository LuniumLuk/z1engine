#pragma once

#include <unordered_map>

#include "core/core.h"
#include "asset/mesh.h"
#include "animation/skeleton.h"
#include "animation/animation.h"

namespace z1 {

	REFLECTED_COMPONENT(StaticMeshComponent) {
		std::shared_ptr<StaticMesh> m_mesh;

		// Per-slot material overrides keyed by material slot name (e.g. "slot0", "slot1").
		// Slots are auto-assigned during mesh import: primitives sharing the same
		// material get the same slot name. Null entries fall through to the
		// primitive's own material.
		std::unordered_map<std::string, std::shared_ptr<MaterialInstance>> m_override_materials;

		StaticMeshComponent(std::shared_ptr<StaticMesh> const& mesh) noexcept
			: m_mesh(mesh) {
			populate_override_slots();
		}

		// Ensure every material slot from the mesh has an entry in the override map.
		// Only adds missing slots (null entries); existing overrides are preserved.
		void populate_override_slots() {
			if (!m_mesh) return;
			for (auto const& prim : m_mesh->m_primitives) {
				std::string slot = prim.m_material_slot.empty() ? "slot0" : prim.m_material_slot;
				m_override_materials.try_emplace(slot);
			}
		}

		// Returns pointer to override map, or nullptr when empty.
		auto const* override_materials_or_null() const { return m_override_materials.empty() ? nullptr : &m_override_materials; }

		DISABLE_COPY(StaticMeshComponent)

	};

	REFLECTED_FIELD(StaticMeshComponent, m_mesh, FF_Default, "[asset]type=static_mesh")
	REFLECTED_FIELD(StaticMeshComponent, m_override_materials, FF_Default, "[asset]type=material_instance")

	REFLECTED_COMPONENT(SkeletalMeshComponent) {
		std::shared_ptr<SkeletalMesh> m_mesh;
		std::shared_ptr<Skeleton> m_skeleton;

		// Per-slot material overrides keyed by material slot name (e.g. "slot0", "slot1").
		// Slots are auto-assigned during mesh import: primitives sharing the same
		// material get the same slot name. Null entries fall through to the
		// primitive's own material.
		std::unordered_map<std::string, std::shared_ptr<MaterialInstance>> m_override_materials;

		SkeletalMeshComponent(
			std::shared_ptr<SkeletalMesh> const& mesh,
			std::shared_ptr<Skeleton> const& skeleton = nullptr) noexcept
			: m_mesh(mesh), m_skeleton(skeleton) {
			populate_override_slots();
		}

		// Ensure every material slot from the mesh has an entry in the override map.
		// Only adds missing slots (null entries); existing overrides are preserved.
		void populate_override_slots() {
			if (!m_mesh) return;
			for (auto const& prim : m_mesh->m_primitives) {
				std::string slot = prim.m_material_slot.empty() ? "slot0" : prim.m_material_slot;
				m_override_materials.try_emplace(slot);
			}
		}

		// Returns pointer to override map, or nullptr when empty.
		auto const* override_materials_or_null() const { return m_override_materials.empty() ? nullptr : &m_override_materials; }

		DISABLE_COPY(SkeletalMeshComponent)

	};

	REFLECTED_FIELD(SkeletalMeshComponent, m_mesh,     FF_Default, "[asset]type=skeletal_mesh")
	REFLECTED_FIELD(SkeletalMeshComponent, m_skeleton, FF_Default, "[asset]type=skeleton")
	REFLECTED_FIELD(SkeletalMeshComponent, m_override_materials, FF_Default, "[asset]type=material_instance")

}
