#ifndef RMT_IO_MESH_EXPORTER_H
#define RMT_IO_MESH_EXPORTER_H

#include <functional>
#include <string>
#include <vector>

#include "rmt/scene/Shape.h"

namespace rmt {

using LogCallback = std::function<void(const std::string&, int)>;

struct Vertex {
    float x, y, z;
    Vertex(float xIn = 0, float yIn = 0, float zIn = 0) : x(xIn), y(yIn), z(zIn) {}
};

struct Triangle {
    int v1, v2, v3;
    Triangle(int v1In, int v2In, int v3In) : v1(v1In), v2(v2In), v3(v3In) {}
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
};

class MeshExporter {
public:
    static void setLogCallback(LogCallback callback);

    static bool exportToOBJ(const std::vector<Shape>& shapes,
                            const std::string& filename,
                            int resolution = 64,
                            float boundingBoxSize = 10.0f);

private:
    static LogCallback logCallback;
    static void log(const std::string& message, int level = 0);

    static Mesh generateMeshFromSDF(const std::vector<Shape>& shapes,
                                    int resolution,
                                    float boundingBoxSize);

    static float sampleSDF(const std::vector<RuntimeShapeData>& runtimeShapes, float x, float y, float z);

    static const int edgeTable[256];
    static const int triTable[256][16];

    static Vertex interpolateVertex(const Vertex& v1, const Vertex& v2,
                                    float val1, float val2, float isolevel);

    static bool writeOBJFile(const Mesh& mesh, const std::string& filename);

    static void calculateBoundingBox(const std::vector<Shape>& shapes,
                                     float& minX, float& minY, float& minZ,
                                     float& maxX, float& maxY, float& maxZ);
};

} // namespace rmt

#endif // RMT_IO_MESH_EXPORTER_H
