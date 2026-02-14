#include "rmt/io/mesh/export/MarchingCubesRunner.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "rmt/io/mesh/export/MeshExporter.h"

namespace rmt {

Mesh MeshExporter::generateMeshFromSDF(const std::vector<Shape>& shapes,
                                       int resolution,
                                       float boundingBoxSize) {
    Mesh mesh;

    float minX = 0.0f;
    float minY = 0.0f;
    float minZ = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    float maxZ = 0.0f;
    calculateBoundingBox(shapes, minX, minY, minZ, maxX, maxY, maxZ);

    const float expandX = (maxX - minX) * 0.2f;
    const float expandY = (maxY - minY) * 0.2f;
    const float expandZ = (maxZ - minZ) * 0.2f;

    minX -= expandX;
    maxX += expandX;
    minY -= expandY;
    maxY += expandY;
    minZ -= expandZ;
    maxZ += expandZ;

    const float stepX = (maxX - minX) / (resolution - 1);
    const float stepY = (maxY - minY) / (resolution - 1);
    const float stepZ = (maxZ - minZ) / (resolution - 1);

    const std::vector<RuntimeShapeData> runtimeShapes = buildRuntimeShapeDataList(shapes);

    const std::size_t gridSize = static_cast<std::size_t>(resolution) *
                                 static_cast<std::size_t>(resolution) *
                                 static_cast<std::size_t>(resolution);
    std::vector<float> sdfValues(gridSize, 0.0f);
    const auto sdfIndex = [resolution](int x, int y, int z) -> std::size_t {
        return (static_cast<std::size_t>(x) * static_cast<std::size_t>(resolution) +
                static_cast<std::size_t>(y)) * static_cast<std::size_t>(resolution) +
               static_cast<std::size_t>(z);
    };

#ifdef RMT_USE_OPENMP
#pragma omp parallel for collapse(3) schedule(static)
#endif
    for (int x = 0; x < resolution; ++x) {
        for (int y = 0; y < resolution; ++y) {
            for (int z = 0; z < resolution; ++z) {
                const float worldX = minX + static_cast<float>(x) * stepX;
                const float worldY = minY + static_cast<float>(y) * stepY;
                const float worldZ = minZ + static_cast<float>(z) * stepZ;
                sdfValues[sdfIndex(x, y, z)] = sampleSDF(runtimeShapes, worldX, worldY, worldZ);
            }
        }
    }

    const float isolevel = 0.0f;

    for (int x = 0; x < resolution - 1; ++x) {
        for (int y = 0; y < resolution - 1; ++y) {
            for (int z = 0; z < resolution - 1; ++z) {
                const float cornerValues[8] = {
                    sdfValues[sdfIndex(x, y, z)],
                    sdfValues[sdfIndex(x + 1, y, z)],
                    sdfValues[sdfIndex(x + 1, y + 1, z)],
                    sdfValues[sdfIndex(x, y + 1, z)],
                    sdfValues[sdfIndex(x, y, z + 1)],
                    sdfValues[sdfIndex(x + 1, y, z + 1)],
                    sdfValues[sdfIndex(x + 1, y + 1, z + 1)],
                    sdfValues[sdfIndex(x, y + 1, z + 1)]
                };

                int cubeIndex = 0;
                for (int i = 0; i < 8; ++i) {
                    if (cornerValues[i] < isolevel) {
                        cubeIndex |= (1 << i);
                    }
                }

                if (edgeTable[cubeIndex] == 0) {
                    continue;
                }

                Vertex edgeVertices[12];
                const Vertex cubeCorners[8] = {
                    Vertex(minX + x * stepX,     minY + y * stepY,     minZ + z * stepZ),
                    Vertex(minX + (x + 1) * stepX, minY + y * stepY,     minZ + z * stepZ),
                    Vertex(minX + (x + 1) * stepX, minY + (y + 1) * stepY, minZ + z * stepZ),
                    Vertex(minX + x * stepX,     minY + (y + 1) * stepY, minZ + z * stepZ),
                    Vertex(minX + x * stepX,     minY + y * stepY,     minZ + (z + 1) * stepZ),
                    Vertex(minX + (x + 1) * stepX, minY + y * stepY,     minZ + (z + 1) * stepZ),
                    Vertex(minX + (x + 1) * stepX, minY + (y + 1) * stepY, minZ + (z + 1) * stepZ),
                    Vertex(minX + x * stepX,     minY + (y + 1) * stepY, minZ + (z + 1) * stepZ)
                };

                const int edgeConnections[12][2] = {
                    {0, 1}, {1, 2}, {2, 3}, {3, 0},
                    {4, 5}, {5, 6}, {6, 7}, {7, 4},
                    {0, 4}, {1, 5}, {2, 6}, {3, 7}
                };

                for (int i = 0; i < 12; ++i) {
                    if (edgeTable[cubeIndex] & (1 << i)) {
                        const int c1 = edgeConnections[i][0];
                        const int c2 = edgeConnections[i][1];
                        edgeVertices[i] = interpolateVertex(cubeCorners[c1], cubeCorners[c2],
                                                            cornerValues[c1], cornerValues[c2], isolevel);
                    }
                }

                for (int i = 0; triTable[cubeIndex][i] != -1; i += 3) {
                    const int baseVertexIndex = static_cast<int>(mesh.vertices.size());
                    mesh.vertices.push_back(edgeVertices[triTable[cubeIndex][i]]);
                    mesh.vertices.push_back(edgeVertices[triTable[cubeIndex][i + 1]]);
                    mesh.vertices.push_back(edgeVertices[triTable[cubeIndex][i + 2]]);
                    mesh.triangles.push_back(Triangle(baseVertexIndex + 2, baseVertexIndex + 1, baseVertexIndex));
                }
            }
        }
    }

    return mesh;
}

float MeshExporter::sampleSDF(const std::vector<RuntimeShapeData>& runtimeShapes, float x, float y, float z) {
    if (runtimeShapes.empty()) {
        return 1.0f;
    }

    const float p[3] = {x, y, z};
    return evaluateRuntimeSceneDistance(p, runtimeShapes);
}

Vertex MeshExporter::interpolateVertex(const Vertex& v1, const Vertex& v2, float val1, float val2, float isolevel) {
    if (std::abs(isolevel - val1) < 1e-6f) {
        return v1;
    }
    if (std::abs(isolevel - val2) < 1e-6f) {
        return v2;
    }
    if (std::abs(val1 - val2) < 1e-6f) {
        return v1;
    }

    const float t = (isolevel - val1) / (val2 - val1);
    return Vertex(
        v1.x + t * (v2.x - v1.x),
        v1.y + t * (v2.y - v1.y),
        v1.z + t * (v2.z - v1.z)
    );
}

} // namespace rmt
