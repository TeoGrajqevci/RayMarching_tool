# Module Map

This refactor introduces canonical headers under `include/rmt/...` and matching source folders under `src/...`.

## Domains

- `app`: runtime orchestration and entrypoint delegation.
- `common`: reusable math/hash/io/fs utilities.
- `scene`: runtime shape data building, primitive distance evaluation, modifiers, blending, and picking.
- `rendering`: GL renderer, shader/texture abstractions.
- `input`: keyboard/mouse interaction and transform tools.
- `ui`: workspace layout, panels, gizmo control, and console/theme integration.
- `io`: mesh import/export and mesh SDF registry.
- `platform`: backend glue (`ImGuiLayer`, GLFW callbacks, file-drop queue).

## Compatibility Layer

Legacy top-level headers in `include/*.h` are temporary wrappers that forward to `include/rmt/...` and re-expose names for existing includes.

## Build Layout

Source manifests live in `cmake/sources_*.cmake`, grouped by domain and included by root `CMakeLists.txt`.
