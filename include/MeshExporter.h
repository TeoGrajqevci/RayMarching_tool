#ifndef MESH_EXPORTER_H
#define MESH_EXPORTER_H

#include "Shapes.h"
#include <vector>
#include <string>

struct Vertex {
    float x, y, z;
    Vertex(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};

struct Triangle {
    int v1, v2, v3;
    Triangle(int v1, int v2, int v3) : v1(v1), v2(v2), v3(v3) {}
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
};

class MeshExporter {
public:
    // Export shapes to OBJ format using marching cubes
    static bool exportToOBJ(const std::vector<Shape>& shapes, 
                           const std::string& filename,
                           int resolution = 64,
                           float boundingBoxSize = 10.0f);

private:
    // Marching cubes implementation
    static Mesh generateMeshFromSDF(const std::vector<Shape>& shapes,
                                  int resolution,
                                  float boundingBoxSize);
    
    // Sample SDF at a point considering all shapes and blend operations
    static float sampleSDF(const std::vector<Shape>& shapes, float x, float y, float z);
    
    // Apply blend operations
    static float applyBlendOperation(float d1, float d2, int blendOp, float smoothness);
    
    // Smooth min/max functions for blending
    static float smoothMin(float a, float b, float k);
    static float smoothMax(float a, float b, float k);
    
    // Marching cubes lookup tables
    static const int edgeTable[256];
    static const int triTable[256][16];
    
    // Interpolate vertex position
    static Vertex interpolateVertex(const Vertex& v1, const Vertex& v2, float val1, float val2, float isolevel);
    
    // Write OBJ file
    static bool writeOBJFile(const Mesh& mesh, const std::string& filename);
    
    // Calculate bounding box from shapes
    static void calculateBoundingBox(const std::vector<Shape>& shapes, 
                                   float& minX, float& minY, float& minZ,
                                   float& maxX, float& maxY, float& maxZ);
};

#endif
