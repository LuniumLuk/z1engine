// Inline definitions for ScriptBase template methods.
// This file is included AFTER Entity is fully defined (from entity.h).
//
// The (Entity*)(void*) casts are safe because m_entity (weak_ptr<void>)
// is always created from a weak_ptr<Entity> via set_entity().

#pragma once

namespace z1 {

	template<typename T, typename Dummy>
	bool ScriptBase::has_component() const {
		auto sp = m_entity.lock();
		return ((Entity*)(void*)sp.get())->has_component<T>();
	}

	template<typename T, typename Dummy>
	T& ScriptBase::get_component() const {
		auto sp = m_entity.lock();
		return ((Entity*)(void*)sp.get())->get_component<T>();
	}

	template<typename T, typename... Args, typename Dummy>
	T& ScriptBase::add_component(Args&&... args) {
		auto sp = m_entity.lock();
		return ((Entity*)(void*)sp.get())->add_component<T>(std::forward<Args>(args)...);
	}

	template<typename T, typename Dummy>
	void ScriptBase::remove_component() {
		auto sp = m_entity.lock();
		return ((Entity*)(void*)sp.get())->remove_component<T>();
	}

	template <typename T>
	bool ScriptBase::is_entity_valid() const {
		auto sp = m_entity.lock();
		return sp && ((Entity*)(void*)sp.get())->is_valid();
	}

	template<typename Dummy>
	std::shared_ptr<Entity> ScriptBase::get_entity() const {
		return std::static_pointer_cast<Entity>(m_entity.lock());
	}

}
