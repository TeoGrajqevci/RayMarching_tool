#include "MeshExporter.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <limits>

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

// Triangle table is defined in MarchingCubesTables.cpp

bool MeshExporter::exportToOBJ(const std::vector<Shape>& shapes, 
                               const std::string& filename,
                               int resolution,
                               float boundingBoxSize) {
    if (shapes.empty()) {
        std::cerr << "No shapes to export!" << std::endl;
        return false;
    }
    
    std::cout << "Generating mesh using Marching Cubes..." << std::endl;
    std::cout << "Resolution: " << resolution << "x" << resolution << "x" << resolution << std::endl;
    
    Mesh mesh = generateMeshFromSDF(shapes, resolution, boundingBoxSize);
    
    if (mesh.vertices.empty() || mesh.triangles.empty()) {
        std::cerr << "Generated mesh is empty!" << std::endl;
        return false;
    }
    
    std::cout << "Generated mesh with " << mesh.vertices.size() << " vertices and " 
              << mesh.triangles.size() << " triangles" << std::endl;
    
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
    
    // Pre-compute SDF values for all grid points
    std::vector<std::vector<std::vector<float>>> sdfValues(resolution, 
        std::vector<std::vector<float>>(resolution, 
            std::vector<float>(resolution)));
    
    for (int x = 0; x < resolution; x++) {
        for (int y = 0; y < resolution; y++) {
            for (int z = 0; z < resolution; z++) {
                float worldX = minX + x * stepX;
                float worldY = minY + y * stepY;
                float worldZ = minZ + z * stepZ;
                sdfValues[x][y][z] = sampleSDF(shapes, worldX, worldY, worldZ);
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
                    sdfValues[x][y][z],                 // 0
                    sdfValues[x+1][y][z],               // 1
                    sdfValues[x+1][y+1][z],             // 2
                    sdfValues[x][y+1][z],               // 3
                    sdfValues[x][y][z+1],               // 4
                    sdfValues[x+1][y][z+1],             // 5
                    sdfValues[x+1][y+1][z+1],           // 6
                    sdfValues[x][y+1][z+1]              // 7
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

float MeshExporter::sampleSDF(const std::vector<Shape>& shapes, float x, float y, float z) {
    if (shapes.empty()) return 1.0f;
    
    float p[3] = {x, y, z};
    float result = std::numeric_limits<float>::max();
    bool firstShape = true;
    
    for (const Shape& shape : shapes) {
        float d = std::numeric_limits<float>::max();
        
        // Calculate distance based on shape type
        switch (shape.type) {
            case SHAPE_SPHERE:
                d = sdSphereCPU(p, shape);
                break;
            case SHAPE_BOX:
                d = sdBoxCPU(p, shape);
                break;
            case SHAPE_ROUNDBOX:
                d = sdRoundBoxCPU(p, shape);
                break;
            case SHAPE_TORUS:
                d = sdTorusCPU(p, shape);
                break;
            case SHAPE_CYLINDER:
                d = sdCylinderCPU(p, shape);
                break;
        }
        
        // Apply blend operation
        if (firstShape) {
            result = d;
            firstShape = false;
        } else {
            result = applyBlendOperation(result, d, shape.blendOp, shape.smoothness);
        }
    }
    
    return result;
}

float MeshExporter::applyBlendOperation(float d1, float d2, int blendOp, float smoothness) {
    switch (blendOp) {
        case BLEND_NONE:
            return std::min(d1, d2);
        case BLEND_SMOOTH_UNION:
            return smoothMin(d1, d2, smoothness);
        case BLEND_SMOOTH_SUBTRACTION:
            return smoothMax(d1, -d2, smoothness);
        case BLEND_SMOOTH_INTERSECTION:
            return smoothMax(d1, d2, smoothness);
        default:
            return std::min(d1, d2);
    }
}

float MeshExporter::smoothMin(float a, float b, float k) {
    if (k <= 0.0f) return std::min(a, b);
    float h = std::max(k - std::abs(a - b), 0.0f);
    return std::min(a, b) - h * h * 0.25f / k;
}

float MeshExporter::smoothMax(float a, float b, float k) {
    return -smoothMin(-a, -b, k);
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
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file for writing: " << filename << std::endl;
        return false;
    }
    
    // Write header
    file << "# OBJ file generated by RayMarching Tool\n";
    file << "# Vertices: " << mesh.vertices.size() << "\n";
    file << "# Triangles: " << mesh.triangles.size() << "\n\n";
    
    // Write vertices
    for (const Vertex& v : mesh.vertices) {
        file << "v " << v.x << " " << v.y << " " << v.z << "\n";
    }
    
    file << "\n";
    
    // Write faces (OBJ uses 1-based indexing)
    for (const Triangle& t : mesh.triangles) {
        file << "f " << (t.v1 + 1) << " " << (t.v2 + 1) << " " << (t.v3 + 1) << "\n";
    }
    
    file.close();
    std::cout << "Successfully exported to: " << filename << std::endl;
    return true;
}

void MeshExporter::calculateBoundingBox(const std::vector<Shape>& shapes, 
                                       float& minX, float& minY, float& minZ,
                                       float& maxX, float& maxY, float& maxZ) {
    minX = minY = minZ = std::numeric_limits<float>::max();
    maxX = maxY = maxZ = -std::numeric_limits<float>::max();
    
    for (const Shape& shape : shapes) {
        float shapeMinX, shapeMinY, shapeMinZ;
        float shapeMaxX, shapeMaxY, shapeMaxZ;
        
        // Calculate bounding box for each shape type
        switch (shape.type) {
            case SHAPE_SPHERE: {
                float radius = shape.param[0];
                shapeMinX = shape.center[0] - radius;
                shapeMinY = shape.center[1] - radius;
                shapeMinZ = shape.center[2] - radius;
                shapeMaxX = shape.center[0] + radius;
                shapeMaxY = shape.center[1] + radius;
                shapeMaxZ = shape.center[2] + radius;
                break;
            }
            case SHAPE_BOX:
            case SHAPE_ROUNDBOX: {
                float extraSize = (shape.type == SHAPE_ROUNDBOX) ? shape.extra : 0.0f;
                shapeMinX = shape.center[0] - shape.param[0] - extraSize;
                shapeMinY = shape.center[1] - shape.param[1] - extraSize;
                shapeMinZ = shape.center[2] - shape.param[2] - extraSize;
                shapeMaxX = shape.center[0] + shape.param[0] + extraSize;
                shapeMaxY = shape.center[1] + shape.param[1] + extraSize;
                shapeMaxZ = shape.center[2] + shape.param[2] + extraSize;
                break;
            }
            case SHAPE_TORUS: {
                float majorRadius = shape.param[0];
                float minorRadius = shape.param[1];
                float totalRadius = majorRadius + minorRadius;
                shapeMinX = shape.center[0] - totalRadius;
                shapeMinY = shape.center[1] - minorRadius;
                shapeMinZ = shape.center[2] - totalRadius;
                shapeMaxX = shape.center[0] + totalRadius;
                shapeMaxY = shape.center[1] + minorRadius;
                shapeMaxZ = shape.center[2] + totalRadius;
                break;
            }
            case SHAPE_CYLINDER: {
                float radius = shape.param[0];
                float halfHeight = shape.param[1];
                shapeMinX = shape.center[0] - radius;
                shapeMinY = shape.center[1] - halfHeight;
                shapeMinZ = shape.center[2] - radius;
                shapeMaxX = shape.center[0] + radius;
                shapeMaxY = shape.center[1] + halfHeight;
                shapeMaxZ = shape.center[2] + radius;
                break;
            }
            default:
                continue;
        }
        
        minX = std::min(minX, shapeMinX);
        minY = std::min(minY, shapeMinY);
        minZ = std::min(minZ, shapeMinZ);
        maxX = std::max(maxX, shapeMaxX);
        maxY = std::max(maxY, shapeMaxY);
        maxZ = std::max(maxZ, shapeMaxZ);
    }
    
    // Ensure we have a valid bounding box
    if (minX >= maxX || minY >= maxY || minZ >= maxZ) {
        minX = minY = minZ = -5.0f;
        maxX = maxY = maxZ = 5.0f;
    }
}
