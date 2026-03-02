#ifndef RMT_APP_UNDO_REDO_MANAGER_H
#define RMT_APP_UNDO_REDO_MANAGER_H

#include <cstddef>
#include <vector>

#include "rmt/scene/Shape.h"

namespace rmt {

class UndoRedoManager {
public:
    explicit UndoRedoManager(std::size_t maxSnapshots = 256u);

    void initialize(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes);
    void capture(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes, bool interactionActive);

    bool undo(std::vector<Shape>& shapes, std::vector<int>& selectedShapes);
    bool redo(std::vector<Shape>& shapes, std::vector<int>& selectedShapes);

    bool canUndo() const;
    bool canRedo() const;
    std::size_t snapshotCount() const;
    std::size_t currentIndex() const;

private:
    struct Snapshot {
        std::vector<Shape> shapes;
        std::vector<int> selectedShapes;
        unsigned long long hash;
    };

    std::size_t maxSnapshots;
    std::vector<Snapshot> snapshots;
    std::size_t cursor;
    unsigned long long lastCommittedHash;
    bool initialized;
    bool interactionWasActive;

    static unsigned long long computeStateHash(const std::vector<Shape>& shapes,
                                               const std::vector<int>& selectedShapes);
    static bool areStatesEqual(const Snapshot& snapshot,
                               const std::vector<Shape>& shapes,
                               const std::vector<int>& selectedShapes);
    static bool areShapesEqual(const Shape& lhs, const Shape& rhs);
    static void sanitizeSelection(std::vector<int>& selectedShapes, std::size_t shapeCount);
    void commitSnapshot(const std::vector<Shape>& shapes, const std::vector<int>& selectedShapes,
                        unsigned long long hash);
    void applySnapshot(std::size_t index, std::vector<Shape>& shapes, std::vector<int>& selectedShapes);
};

} // namespace rmt

#endif // RMT_APP_UNDO_REDO_MANAGER_H
