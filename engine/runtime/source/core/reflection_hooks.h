#pragma once

namespace z1 {

	// Force the linker to include reflection_hooks.cpp so its static
	// registrars (REGISTER_COMPONENT_HOOKS, etc.) are not stripped.
	void ForceLinkReflectionHooks();

}
