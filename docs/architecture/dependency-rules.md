# Dependency Rules

## Allowed dependencies

- `common`: STL only.
- `scene`: `common`, mesh SDF registry interface.
- `rendering`: `scene`, `common`, GL/GLFW, optional OIDN.
- `input`: `scene`, `common`, ImGui/GLFW.
- `ui`: `scene`, `input`, `rendering` public snapshots, ImGui.
- `io`: `scene`, `common`, assimp, mesh2sdf.
- `platform`: backend integration only.
- `app`: orchestration layer over all domain modules.

## Guardrails

- New cross-domain include edges should be added only through public headers.
- Temporary compatibility wrappers must not be used by new domain internals.
