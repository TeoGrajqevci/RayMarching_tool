#include "rmt/app/UndoRedoManager.h"
#include "rmt/common/hash/Fnv1a.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace rmt {

namespace {

template <typename T, std::size_t N>
bool arrayEqual(const T (&lhs)[N], const T (&rhs)[N]) {
    for (std::size_t i = 0; i < N; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

} // namespace

UndoRedoManager::UndoRedoManager(std::size_t maxSnapshotsIn)
    : maxSnapshots(std::max<std::size_t>(2u, maxSnapshotsIn)),
      cursor(0u),
      lastCommittedHash(0ULL),
    initialized(false),
    interactionWasActive(false) {}

void UndoRedoManager::initialize(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes) {
    snapshots.clear();
    cursor = 0u;
    initialized = true;
    interactionWasActive = false;

    const std::uint64_t hash = computeStateHash(shapes, selectedShapes);
    Snapshot snapshot;
    snapshot.shapes = shapes;
    snapshot.selectedShapes = selectedShapes;
    sanitizeSelection(snapshot.selectedShapes, snapshot.shapes.size());
    snapshot.hash = hash;
    snapshots.push_back(snapshot);
    lastCommittedHash = hash;
}

void UndoRedoManager::capture(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes, bool interactionActive) {
    if (!initialized) {
        initialize(shapes, selectedShapes);
        return;
    }

    const std::uint64_t hash = computeStateHash(shapes, selectedShapes);
    const bool stateChanged = snapshots.empty() ? true : !areStatesEqual(snapshots[cursor], shapes, selectedShapes);

    if (interactionActive) {
        interactionWasActive = true;
        return;
    }

    if (interactionWasActive) {
        interactionWasActive = false;
        if (!stateChanged) {
            return;
        }
        commitSnapshot(shapes, selectedShapes, hash);
        return;
    }

    if (!stateChanged) {
        return;
    }

    commitSnapshot(shapes, selectedShapes, hash);
}

bool UndoRedoManager::undo(std::vector<Shape>& shapes, std::vector<int>& selectedShapes) {
    if (!canUndo()) {
        return false;
    }

    if (cursor == 0u) {
        return false;
    }

    applySnapshot(cursor - 1u, shapes, selectedShapes);
    return true;
}

bool UndoRedoManager::redo(std::vector<Shape>& shapes, std::vector<int>& selectedShapes) {
    if (!canRedo()) {
        return false;
    }

    applySnapshot(cursor + 1u, shapes, selectedShapes);
    return true;
}

bool UndoRedoManager::canUndo() const {
    return initialized && !snapshots.empty() && cursor > 0u;
}

bool UndoRedoManager::canRedo() const {
    return initialized && !snapshots.empty() && (cursor + 1u) < snapshots.size();
}

std::size_t UndoRedoManager::snapshotCount() const {
    return snapshots.size();
}

std::size_t UndoRedoManager::currentIndex() const {
    return cursor;
}

unsigned long long UndoRedoManager::computeStateHash(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes) {
    std::uint64_t hash = kFnv1aOffsetBasis;

    const std::size_t shapeCount = shapes.size();
    fnv1aHashValue(hash, shapeCount);
    for (std::size_t i = 0u; i < shapes.size(); ++i) {
        const Shape& shape = shapes[i];
        fnv1aHashValue(hash, shape.type);
        fnv1aHashRaw(hash, shape.center, sizeof(shape.center));
        fnv1aHashRaw(hash, shape.param, sizeof(shape.param));
        fnv1aHashRaw(hash, shape.fractalExtra, sizeof(shape.fractalExtra));
        fnv1aHashRaw(hash, shape.rotation, sizeof(shape.rotation));
        fnv1aHashValue(hash, shape.extra);
        fnv1aHashRaw(hash, shape.scale, sizeof(shape.scale));
        fnv1aHashValue(hash, shape.scaleMode);
        fnv1aHashRaw(hash, shape.elongation, sizeof(shape.elongation));
        fnv1aHashRaw(hash, shape.twist, sizeof(shape.twist));
        fnv1aHashRaw(hash, shape.bend, sizeof(shape.bend));
        fnv1aHashString(hash, shape.name);
        fnv1aHashValue(hash, shape.blendOp);
        fnv1aHashValue(hash, shape.smoothness);
        fnv1aHashRaw(hash, shape.albedo, sizeof(shape.albedo));
        fnv1aHashValue(hash, shape.metallic);
        fnv1aHashValue(hash, shape.roughness);
        fnv1aHashString(hash, shape.albedoTexturePath);
        fnv1aHashString(hash, shape.roughnessTexturePath);
        fnv1aHashString(hash, shape.metallicTexturePath);
        fnv1aHashString(hash, shape.normalTexturePath);
        fnv1aHashString(hash, shape.displacementTexturePath);
        fnv1aHashValue(hash, shape.displacementStrength);
        fnv1aHashRaw(hash, shape.emission, sizeof(shape.emission));
        fnv1aHashValue(hash, shape.emissionStrength);
        fnv1aHashValue(hash, shape.transmission);
        fnv1aHashValue(hash, shape.ior);
        fnv1aHashValue(hash, shape.dispersion);
        fnv1aHashValue(hash, shape.bendModifierEnabled);
        fnv1aHashValue(hash, shape.twistModifierEnabled);
        fnv1aHashValue(hash, shape.mirrorModifierEnabled);
        fnv1aHashRaw(hash, shape.mirrorAxis, sizeof(shape.mirrorAxis));
        fnv1aHashRaw(hash, shape.mirrorOffset, sizeof(shape.mirrorOffset));
        fnv1aHashValue(hash, shape.mirrorSmoothness);
    }

    const std::size_t selectedCount = selectedShapes.size();
    fnv1aHashValue(hash, selectedCount);
    for (std::size_t i = 0u; i < selectedShapes.size(); ++i) {
        fnv1aHashValue(hash, selectedShapes[i]);
    }

    return hash;
}

bool UndoRedoManager::areShapesEqual(const Shape& lhs, const Shape& rhs) {
    if (lhs.type != rhs.type ||
        lhs.extra != rhs.extra ||
        lhs.scaleMode != rhs.scaleMode ||
        lhs.name != rhs.name ||
        lhs.meshSourcePath != rhs.meshSourcePath ||
        lhs.blendOp != rhs.blendOp ||
        lhs.smoothness != rhs.smoothness ||
        lhs.metallic != rhs.metallic ||
        lhs.roughness != rhs.roughness ||
        lhs.albedoTexturePath != rhs.albedoTexturePath ||
        lhs.roughnessTexturePath != rhs.roughnessTexturePath ||
        lhs.metallicTexturePath != rhs.metallicTexturePath ||
        lhs.normalTexturePath != rhs.normalTexturePath ||
        lhs.displacementTexturePath != rhs.displacementTexturePath ||
        lhs.displacementStrength != rhs.displacementStrength ||
        lhs.emissionStrength != rhs.emissionStrength ||
        lhs.transmission != rhs.transmission ||
        lhs.ior != rhs.ior ||
        lhs.dispersion != rhs.dispersion ||
        lhs.bendModifierEnabled != rhs.bendModifierEnabled ||
        lhs.twistModifierEnabled != rhs.twistModifierEnabled ||
        lhs.mirrorModifierEnabled != rhs.mirrorModifierEnabled ||
        lhs.mirrorSmoothness != rhs.mirrorSmoothness ||
        lhs.arrayModifierEnabled != rhs.arrayModifierEnabled ||
        lhs.arraySmoothness != rhs.arraySmoothness) {
        return false;
    }

    if (!arrayEqual(lhs.center, rhs.center) ||
        !arrayEqual(lhs.param, rhs.param) ||
        !arrayEqual(lhs.fractalExtra, rhs.fractalExtra) ||
        !arrayEqual(lhs.rotation, rhs.rotation) ||
        !arrayEqual(lhs.scale, rhs.scale) ||
        !arrayEqual(lhs.elongation, rhs.elongation) ||
        !arrayEqual(lhs.twist, rhs.twist) ||
        !arrayEqual(lhs.bend, rhs.bend) ||
        !arrayEqual(lhs.albedo, rhs.albedo) ||
        !arrayEqual(lhs.emission, rhs.emission) ||
        !arrayEqual(lhs.mirrorAxis, rhs.mirrorAxis) ||
        !arrayEqual(lhs.mirrorOffset, rhs.mirrorOffset) ||
        !arrayEqual(lhs.arrayAxis, rhs.arrayAxis) ||
        !arrayEqual(lhs.arraySpacing, rhs.arraySpacing) ||
        !arrayEqual(lhs.arrayRepeatCount, rhs.arrayRepeatCount) ||
        !arrayEqual(lhs.modifierStack, rhs.modifierStack)) {
        return false;
    }

    if (lhs.curveNodes.size() != rhs.curveNodes.size()) {
        return false;
    }
    for (std::size_t i = 0; i < lhs.curveNodes.size(); ++i) {
        if (!arrayEqual(lhs.curveNodes[i].position, rhs.curveNodes[i].position) ||
            lhs.curveNodes[i].radius != rhs.curveNodes[i].radius) {
            return false;
        }
    }

    return true;
}

bool UndoRedoManager::areStatesEqual(const Snapshot& snapshot,
                                     const std::vector<Shape>& shapes,
                                     const std::vector<int>& selectedShapes) {
    if (snapshot.shapes.size() != shapes.size() ||
        snapshot.selectedShapes.size() != selectedShapes.size()) {
        return false;
    }

    for (std::size_t i = 0; i < shapes.size(); ++i) {
        if (!areShapesEqual(snapshot.shapes[i], shapes[i])) {
            return false;
        }
    }

    for (std::size_t i = 0; i < selectedShapes.size(); ++i) {
        if (snapshot.selectedShapes[i] != selectedShapes[i]) {
            return false;
        }
    }

    return true;
}

void UndoRedoManager::sanitizeSelection(std::vector<int>& selectedShapes, std::size_t shapeCount) {
    selectedShapes.erase(
        std::remove_if(selectedShapes.begin(), selectedShapes.end(),
                       [shapeCount](int index) {
                           return index < 0 || static_cast<std::size_t>(index) >= shapeCount;
                       }),
        selectedShapes.end());
}

void UndoRedoManager::commitSnapshot(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes, unsigned long long hash) {
    if (!initialized) {
        initialize(shapes, selectedShapes);
        return;
    }

    if (cursor + 1u < snapshots.size()) {
        snapshots.erase(snapshots.begin() + static_cast<std::ptrdiff_t>(cursor + 1u), snapshots.end());
    }

    if (snapshots.size() >= maxSnapshots) {
        snapshots.erase(snapshots.begin());
        if (cursor > 0u) {
            --cursor;
        }
    }

    Snapshot snapshot;
    snapshot.shapes = shapes;
    snapshot.selectedShapes = selectedShapes;
    sanitizeSelection(snapshot.selectedShapes, snapshot.shapes.size());
    snapshot.hash = hash;
    snapshots.push_back(snapshot);
    cursor = snapshots.size() - 1u;
    lastCommittedHash = hash;
}

void UndoRedoManager::applySnapshot(std::size_t index, std::vector<Shape>& shapes, std::vector<int>& selectedShapes) {
    if (index >= snapshots.size()) {
        return;
    }

    cursor = index;
    const Snapshot& snapshot = snapshots[cursor];
    shapes = snapshot.shapes;
    selectedShapes = snapshot.selectedShapes;
    sanitizeSelection(selectedShapes, shapes.size());
    lastCommittedHash = snapshot.hash;
}

} // namespace rmt
