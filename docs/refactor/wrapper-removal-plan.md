# Wrapper Removal Plan

Legacy wrappers in `include/*.h` are temporary.

## Current state

- Canonical headers: `include/rmt/...`
- Compatibility wrappers: `include/*.h` forwarding to canonical headers.

## Removal criteria

1. All internal source files include `rmt/...` paths only.
2. External call sites (if any) are migrated.
3. A release note documents removal of legacy include paths.

## Removal steps

1. Replace any remaining includes of legacy wrappers.
2. Delete wrapper headers from `include/*.h`.
3. Run full build + benchmark smoke gates.
