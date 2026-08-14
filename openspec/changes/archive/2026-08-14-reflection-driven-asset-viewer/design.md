## Context

The editor's asset panel was historically hand-written per asset type. The reflection-system upgrade (`c3962da`) provided field metadata, so the asset viewer can be data-driven.

## Goals / Non-Goals

**Goals:**
- One reflection-driven browser + inspector for all asset types.
- Editable reflected fields with save-back for writable assets.

**Non-Goals:**
- Type-specific custom widgets beyond reflection hints (handled by widget-hint metadata).

## Decisions

### D1. Browser + inspector driven by AssetMeta and reflection
`m_browser->m_on_asset_opened` opens an asset into `m_selected_asset`; `show_asset_info()` renders its reflected fields; drag-drop of `ASSET_ITEM` payloads wires assets into scene components.

### D2. Save reuses the reflection serializer
Asset save calls the same `serialize_field` path used by scene serialization so asset types stay in sync automatically.

## Risks / Trade-offs

- [Generic UI less tailored than bespoke panels] → Acceptable: reflection widget hints provide per-field customization.

## Migration Plan

Already shipped; no migration required.
