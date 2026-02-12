#include "UndoRedo.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {

static const std::uint64_t kFnvOffsetBasis = 1469598103934665603ULL;
static const std::uint64_t kFnvPrime = 1099511628211ULL;

void hashRaw(std::uint64_t& hash, const void* data, std::size_t sizeBytes) {
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < sizeBytes; ++i) {
        hash ^= static_cast<std::uint64_t>(bytes[i]);
        hash *= kFnvPrime;
    }
}

template <typename T>
void hashValue(std::uint64_t& hash, const T& value) {
    hashRaw(hash, &value, sizeof(T));
}

void hashString(std::uint64_t& hash, const std::string& value) {
    const std::size_t length = value.size();
    hashValue(hash, length);
    if (!value.empty()) {
        hashRaw(hash, value.data(), value.size());
    }
}

} // namespace

UndoRedoManager::UndoRedoManager(std::size_t maxSnapshotsIn)
    : maxSnapshots(std::max<std::size_t>(2u, maxSnapshotsIn)),
      cursor(0u),
      lastCommittedHash(0ULL),
      initialized(false) {}

void UndoRedoManager::initialize(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes) {
    snapshots.clear();
    cursor = 0u;
    initialized = true;

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

    if (interactionActive) {
        return;
    }

    const std::uint64_t hash = computeStateHash(shapes, selectedShapes);
    if (hash == lastCommittedHash) {
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

unsigned long long UndoRedoManager::computeStateHash(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes) {
    std::uint64_t hash = kFnvOffsetBasis;

    const std::size_t shapeCount = shapes.size();
    hashValue(hash, shapeCount);
    for (std::size_t i = 0u; i < shapes.size(); ++i) {
        const Shape& shape = shapes[i];
        hashValue(hash, shape.type);
        hashRaw(hash, shape.center, sizeof(shape.center));
        hashRaw(hash, shape.param, sizeof(shape.param));
        hashRaw(hash, shape.rotation, sizeof(shape.rotation));
        hashValue(hash, shape.extra);
        hashRaw(hash, shape.scale, sizeof(shape.scale));
        hashValue(hash, shape.scaleMode);
        hashRaw(hash, shape.elongation, sizeof(shape.elongation));
        hashRaw(hash, shape.twist, sizeof(shape.twist));
        hashRaw(hash, shape.bend, sizeof(shape.bend));
        hashString(hash, shape.name);
        hashValue(hash, shape.blendOp);
        hashValue(hash, shape.smoothness);
        hashRaw(hash, shape.albedo, sizeof(shape.albedo));
        hashValue(hash, shape.metallic);
        hashValue(hash, shape.roughness);
        hashRaw(hash, shape.emission, sizeof(shape.emission));
        hashValue(hash, shape.emissionStrength);
        hashValue(hash, shape.transmission);
        hashValue(hash, shape.ior);
        hashValue(hash, shape.bendModifierEnabled);
        hashValue(hash, shape.twistModifierEnabled);
        hashValue(hash, shape.mirrorModifierEnabled);
        hashRaw(hash, shape.mirrorAxis, sizeof(shape.mirrorAxis));
        hashRaw(hash, shape.mirrorOffset, sizeof(shape.mirrorOffset));
        hashValue(hash, shape.mirrorSmoothness);
    }

    const std::size_t selectedCount = selectedShapes.size();
    hashValue(hash, selectedCount);
    for (std::size_t i = 0u; i < selectedShapes.size(); ++i) {
        hashValue(hash, selectedShapes[i]);
    }

    return hash;
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
