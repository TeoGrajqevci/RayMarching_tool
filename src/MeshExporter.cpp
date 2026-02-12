#include "MeshExporter.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <limits>
#include <chrono>

// Marching Cubes edge table - which edges are intersected for each configuration
const int MeshExporter::edgeTable[256] = {
    0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
    0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
    0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
    0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
    0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
    0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
    0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
    0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
    0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
    0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
    0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
    0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
    0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
    0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
    0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
    0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
    0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
    0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
    0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
    0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
    0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
    0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
    0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
    0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
    0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
    0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
    0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
    0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
    0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
    0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
    0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
    0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
};

// Static member definition
LogCallback MeshExporter::logCallback = nullptr;

void MeshExporter::setLogCallback(LogCallback callback) {
    logCallback = callback;
}

void MeshExporter::log(const std::string& message, int level) {
    if (logCallback) {
        logCallback(message, level);
    } else {
        // Fallback to console output
        if (level == 2) {
            std::cerr << message << std::endl;
        } else {
            std::cout << message << std::endl;
        }
    }
}

// Triangle table is defined in MarchingCubesTables.cpp

bool MeshExporter::exportToOBJ(const std::vector<Shape>& shapes, 
                               const std::string& filename,
                               int resolution,
                               float boundingBoxSize) {
    if (shapes.empty()) {
        std::cerr << "❌ No shapes to export!" << std::endl;
        return false;
    }
    
    if (resolution < 8 || resolution > 1024) {
        log("❌ Invalid resolution: " + std::to_string(resolution) + " (must be between 8 and 1024)", 2);
        return false;
    }
    
    if (filename.empty()) {
        log("❌ Empty filename provided!", 2);
        return false;
    }
    
    log("🔄 Generating mesh using Marching Cubes...", 0);
    log("📊 Resolution: " + std::to_string(resolution) + "x" + std::to_string(resolution) + "x" + std::to_string(resolution) 
        + " (" + std::to_string(resolution * resolution * resolution) + " voxels)", 0);
    log("📦 Bounding box size: " + std::to_string(boundingBoxSize), 0);
    log("🎯 Number of shapes: " + std::to_string(shapes.size()), 0);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    Mesh mesh = generateMeshFromSDF(shapes, resolution, boundingBoxSize);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    if (mesh.vertices.empty() || mesh.triangles.empty()) {
        log("❌ Generated mesh is empty! Try adjusting resolution or bounding box size.", 2);
        return false;
    }
    
    log("✅ Generated mesh with " + std::to_string(mesh.vertices.size()) + " vertices and " 
        + std::to_string(mesh.triangles.size()) + " triangles", 0);
    log("⏱️  Generation time: " + std::to_string(duration.count()) + " ms", 0);
    
    return writeOBJFile(mesh, filename);
}

Mesh MeshExporter::generateMeshFromSDF(const std::vector<Shape>& shapes,
                                      int resolution,
                                      float boundingBoxSize) {
    Mesh mesh;
    
    // Calculate actual bounding box from shapes
    float minX, minY, minZ, maxX, maxY, maxZ;
    calculateBoundingBox(shapes, minX, minY, minZ, maxX, maxY, maxZ);
    
    // Expand bounding box by 20% to ensure we capture the full shape
    float expandX = (maxX - minX) * 0.2f;
    float expandY = (maxY - minY) * 0.2f;
    float expandZ = (maxZ - minZ) * 0.2f;
    
    minX -= expandX; maxX += expandX;
    minY -= expandY; maxY += expandY;
    minZ -= expandZ; maxZ += expandZ;
    
    float stepX = (maxX - minX) / (resolution - 1);
    float stepY = (maxY - minY) / (resolution - 1);
    float stepZ = (maxZ - minZ) / (resolution - 1);

    const std::vector<RuntimeShapeData> runtimeShapes = buildRuntimeShapeDataList(shapes);

    // Pre-compute SDF values for all grid points in a contiguous buffer.
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
    
    // Apply Marching Cubes algorithm
    const float isolevel = 0.0f;
    
    for (int x = 0; x < resolution - 1; x++) {
        for (int y = 0; y < resolution - 1; y++) {
            for (int z = 0; z < resolution - 1; z++) {
                // Get the 8 corner values of the cube
                float cornerValues[8] = {
                    sdfValues[sdfIndex(x, y, z)],             // 0
                    sdfValues[sdfIndex(x + 1, y, z)],         // 1
                    sdfValues[sdfIndex(x + 1, y + 1, z)],     // 2
                    sdfValues[sdfIndex(x, y + 1, z)],         // 3
                    sdfValues[sdfIndex(x, y, z + 1)],         // 4
                    sdfValues[sdfIndex(x + 1, y, z + 1)],     // 5
                    sdfValues[sdfIndex(x + 1, y + 1, z + 1)], // 6
                    sdfValues[sdfIndex(x, y + 1, z + 1)]      // 7
                };
                
                // Get cube configuration
                int cubeIndex = 0;
                for (int i = 0; i < 8; i++) {
                    if (cornerValues[i] < isolevel) {
                        cubeIndex |= (1 << i);
                    }
                }
                
                // Skip if cube is entirely inside or outside
                if (edgeTable[cubeIndex] == 0) continue;
                
                // Calculate edge intersection points
                Vertex edgeVertices[12];
                Vertex cubeCorners[8] = {
                    Vertex(minX + x * stepX,     minY + y * stepY,     minZ + z * stepZ),       // 0
                    Vertex(minX + (x+1) * stepX, minY + y * stepY,     minZ + z * stepZ),       // 1
                    Vertex(minX + (x+1) * stepX, minY + (y+1) * stepY, minZ + z * stepZ),       // 2
                    Vertex(minX + x * stepX,     minY + (y+1) * stepY, minZ + z * stepZ),       // 3
                    Vertex(minX + x * stepX,     minY + y * stepY,     minZ + (z+1) * stepZ),   // 4
                    Vertex(minX + (x+1) * stepX, minY + y * stepY,     minZ + (z+1) * stepZ),   // 5
                    Vertex(minX + (x+1) * stepX, minY + (y+1) * stepY, minZ + (z+1) * stepZ),   // 6
                    Vertex(minX + x * stepX,     minY + (y+1) * stepY, minZ + (z+1) * stepZ)    // 7
                };
                
                // Edge connections (which corners are connected by each edge)
                int edgeConnections[12][2] = {
                    {0,1}, {1,2}, {2,3}, {3,0},  // bottom face
                    {4,5}, {5,6}, {6,7}, {7,4},  // top face
                    {0,4}, {1,5}, {2,6}, {3,7}   // vertical edges
                };
                
                // Calculate intersection points for edges that are intersected
                for (int i = 0; i < 12; i++) {
                    if (edgeTable[cubeIndex] & (1 << i)) {
                        int c1 = edgeConnections[i][0];
                        int c2 = edgeConnections[i][1];
                        edgeVertices[i] = interpolateVertex(cubeCorners[c1], cubeCorners[c2], 
                                                          cornerValues[c1], cornerValues[c2], isolevel);
                    }
                }
                
                // Generate triangles based on the configuration
                for (int i = 0; triTable[cubeIndex][i] != -1; i += 3) {
                    int baseVertexIndex = mesh.vertices.size();
                    
                    // Add vertices
                    mesh.vertices.push_back(edgeVertices[triTable[cubeIndex][i]]);
                    mesh.vertices.push_back(edgeVertices[triTable[cubeIndex][i+1]]);
                    mesh.vertices.push_back(edgeVertices[triTable[cubeIndex][i+2]]);
                    
                    // Add triangle (note: reverse winding order for correct normals)
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

    const float p[3] = { x, y, z };
    return evaluateRuntimeSceneDistance(p, runtimeShapes);
}

Vertex MeshExporter::interpolateVertex(const Vertex& v1, const Vertex& v2, float val1, float val2, float isolevel) {
    if (std::abs(isolevel - val1) < 1e-6f) return v1;
    if (std::abs(isolevel - val2) < 1e-6f) return v2;
    if (std::abs(val1 - val2) < 1e-6f) return v1;
    
    float t = (isolevel - val1) / (val2 - val1);
    return Vertex(
        v1.x + t * (v2.x - v1.x),
        v1.y + t * (v2.y - v1.y),
        v1.z + t * (v2.z - v1.z)
    );
}

bool MeshExporter::writeOBJFile(const Mesh& mesh, const std::string& filename) {
    log("💾 Writing OBJ file: " + filename, 0);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        log("❌ Could not open file for writing: " + filename, 2);
        log("   Check if the directory exists and you have write permissions.", 2);
        return false;
    }
    
    // Write header
    file << "# OBJ file generated by RayMarching Tool\n";
    file << "# Generated on: " << __DATE__ << " " << __TIME__ << "\n";
    file << "# Vertices: " << mesh.vertices.size() << "\n";
    file << "# Triangles: " << mesh.triangles.size() << "\n";
    file << "# \n";
    file << "# This mesh was generated using the Marching Cubes algorithm\n";
    file << "# from Signed Distance Field (SDF) data\n\n";
    
    // Write vertices
    for (const Vertex& v : mesh.vertices) {
        file << "v " << v.x << " " << v.y << " " << v.z << "\n";
    }
    
    file << "\n# Faces (triangles)\n";
    
    // Write faces (OBJ uses 1-based indexing)
    for (const Triangle& t : mesh.triangles) {
        file << "f " << (t.v1 + 1) << " " << (t.v2 + 1) << " " << (t.v3 + 1) << "\n";
    }
    
    file.close();
    
    // Verify file was written successfully
    std::ifstream checkFile(filename);
    if (!checkFile.is_open()) {
        log("❌ Failed to verify exported file!", 2);
        return false;
    }
    checkFile.close();
    
    log("✅ Successfully exported to: " + filename, 0);
    log("📊 File contains " + std::to_string(mesh.vertices.size()) + " vertices and " 
        + std::to_string(mesh.triangles.size()) + " triangles", 0);
    
    return true;
}

void MeshExporter::calculateBoundingBox(const std::vector<Shape>& shapes, 
                                       float& minX, float& minY, float& minZ,
                                       float& maxX, float& maxY, float& maxZ) {
    minX = minY = minZ = std::numeric_limits<float>::max();
    maxX = maxY = maxZ = -std::numeric_limits<float>::max();
    
    for (const Shape& shape : shapes) {
        float baseRadius = 0.0f;
        switch (shape.type) {
            case SHAPE_SPHERE: {
                baseRadius = shape.param[0];
                break;
            }
            case SHAPE_BOX: {
                baseRadius = std::sqrt(
                    shape.param[0] * shape.param[0] +
                    shape.param[1] * shape.param[1] +
                    shape.param[2] * shape.param[2]
                );
                break;
            }
            case SHAPE_TORUS: {
                baseRadius = shape.param[0] + shape.param[1];
                break;
            }
            case SHAPE_CYLINDER: {
                baseRadius = std::sqrt(shape.param[0] * shape.param[0] + shape.param[1] * shape.param[1]);
                break;
            }
            case SHAPE_CONE: {
                baseRadius = std::sqrt(shape.param[0] * shape.param[0] + shape.param[1] * shape.param[1]);
                break;
            }
            case SHAPE_MANDELBULB: {
                baseRadius = std::max(1.5f, std::max(shape.param[2], 2.0f) * 0.5f);
                break;
            }
            default:
                continue;
        }

        float maxScale = std::max(
            std::fabs(shape.scale[0]),
            std::max(std::fabs(shape.scale[1]), std::fabs(shape.scale[2]))
        );
        baseRadius *= std::max(maxScale, 0.001f);

        float eX = std::max(shape.elongation[0], 0.0f);
        float eY = std::max(shape.elongation[1], 0.0f);
        float eZ = std::max(shape.elongation[2], 0.0f);
        baseRadius += std::sqrt(eX * eX + eY * eY + eZ * eZ);

        baseRadius += std::max(shape.extra, 0.0f);
        float warpBoost = 1.0f + 0.5f * (
            std::fabs(shape.twist[0]) + std::fabs(shape.twist[1]) + std::fabs(shape.twist[2]) +
            std::fabs(shape.bend[0]) + std::fabs(shape.bend[1]) + std::fabs(shape.bend[2])
        );
        float extent = baseRadius * warpBoost + 0.01f;
        if (shape.mirrorModifierEnabled) {
            extent += std::max(shape.mirrorSmoothness, 0.0f);
        }
        const int maxMirrorX = (shape.mirrorModifierEnabled && shape.mirrorAxis[0]) ? 1 : 0;
        const int maxMirrorY = (shape.mirrorModifierEnabled && shape.mirrorAxis[1]) ? 1 : 0;
        const int maxMirrorZ = (shape.mirrorModifierEnabled && shape.mirrorAxis[2]) ? 1 : 0;

        for (int ix = 0; ix <= maxMirrorX; ++ix) {
            for (int iy = 0; iy <= maxMirrorY; ++iy) {
                for (int iz = 0; iz <= maxMirrorZ; ++iz) {
                    const float cx = (ix == 1) ? (2.0f * shape.mirrorOffset[0] - shape.center[0]) : shape.center[0];
                    const float cy = (iy == 1) ? (2.0f * shape.mirrorOffset[1] - shape.center[1]) : shape.center[1];
                    const float cz = (iz == 1) ? (2.0f * shape.mirrorOffset[2] - shape.center[2]) : shape.center[2];

                    const float shapeMinX = cx - extent;
                    const float shapeMinY = cy - extent;
                    const float shapeMinZ = cz - extent;
                    const float shapeMaxX = cx + extent;
                    const float shapeMaxY = cy + extent;
                    const float shapeMaxZ = cz + extent;

                    minX = std::min(minX, shapeMinX);
                    minY = std::min(minY, shapeMinY);
                    minZ = std::min(minZ, shapeMinZ);
                    maxX = std::max(maxX, shapeMaxX);
                    maxY = std::max(maxY, shapeMaxY);
                    maxZ = std::max(maxZ, shapeMaxZ);
                }
            }
        }
    }
    
    // Ensure we have a valid bounding box
    if (minX >= maxX || minY >= maxY || minZ >= maxZ) {
        minX = minY = minZ = -5.0f;
        maxX = maxY = maxZ = 5.0f;
    }
}
