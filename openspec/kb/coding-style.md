# Coding Style
> Summary: Enforceable code formatting, naming, const, include, and prohibited pattern rules
> Scope: engine/runtime/, engine/editor/, engine/bakery/, engine/test/

## Formatting

| Rule | Convention |
|------|-----------|
| Indentation | Hard tabs (not spaces), tab width = 4 |
| Brace style | K&R (opening brace on same line): `if (cond) {`, `void foo() {` |
| Line endings | CRLF (Windows) |
| Column limit | 120 columns (soft limit) |
| Header guards | `#pragma once` (not `#ifndef`) |

## Naming

| Element | Convention | Example |
|---------|-----------|---------|
| Types (class, struct, enum) | PascalCase | `RenderGraph`, `MeshComponent` |
| Methods / functions | camelCase with underscores | `on_update`, `push_layer`, `create_frustum` |
| Member variables | `m_` prefix | `m_window`, `m_timer`, `m_should_exit` |
| File names | lowercase + underscores | `render_graph.cpp`, `vertex_array.h` |
| Test files | `test_<name>.cpp` | `test_render_graph.cpp` |
| Namespace | `z1` (or nested: `z1::bakery`) | `namespace z1 {` |
| Constants / macros | ALL_CAPS for macros | `PROFILE_FUNCTION()` |

## Const and Type Usage

- **East-const**: `Type const&` (e.g., `std::string const& name`, `Filepath const& path`)
- **Smart pointer passing**: `std::shared_ptr<T> const&`
- **Ownership**: `std::shared_ptr` for shared, `std::unique_ptr` for unique
- **Raw pointers**: non-owning references only
- **`noexcept`**: on small helper functions (follow existing patterns)

## Include Order (`.cpp` files)

1. `#include "pch.h"` (first, mandatory)
2. Corresponding header (`foo.cpp` -> `"foo.h"`)
3. Same-module engine headers (`"..."`)
4. Other engine module headers (`"..."`)
5. Third-party headers (`<...>`)
6. Standard library headers (`<...>`)
- Groups separated by blank line

## Comments

| Rule | Convention |
|------|-----------|
| Line comments | ≤ 1 line |
| Module / class comments | ≤ 3 lines |
| Multi-line comments | Only for whole-system or concept explanations |

## Prohibited Patterns

| Pattern | Reason |
|---------|--------|
| `using namespace std;` | Pollutes namespace |
| `std::endl` | Use `"\n"` or `'\n'` |
| `std::cerr` / `std::cout` for diagnostics | Use `CORE_DEBUG`, `CORE_WARN`, `CORE_ERROR` |
| Raw owning `new`/`delete` | Use smart pointers |
| `#ifndef`/`#define` guards | Use `#pragma once` |
| Editing `engine/3rdparty/` | Requires explicit authorization |

## Logging Macros

| Macro | Use |
|-------|-----|
| `CORE_DEBUG(...)` | Debug info (stripped in Release) |
| `CORE_WARN(...)` | Warnings |
| `CORE_ERROR(...)` | Errors |
| `CLIENT_INFO(...)` | Client-facing info |
| `PROFILE_FUNCTION()` | Profile a function |
| `PROFILE_SCOPE("name")` | Profile a code block |

## API Macro

- Exported types use `struct API TypeName` pattern
- Maintain `API` macro on exported types when adding new ones

## Pre-Commit Checklist

- [ ] Hard tabs, K&R braces
- [ ] `pch.h` first in `.cpp` files
- [ ] `#pragma once` in headers
- [ ] PascalCase types, camelCase functions, `m_` members
- [ ] East-const: `Type const&`
- [ ] No prohibited patterns introduced
- [ ] No edits to `engine/3rdparty/`
- [ ] DCV loop passed (COMPILE at minimum)

-> see [contributing.md#commits]
