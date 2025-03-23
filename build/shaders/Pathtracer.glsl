#version 330 core
out vec4 FragColor;
in vec2 fragCoord;
uniform vec2 iResolution;
uniform float iTime;

uniform float shapeMetallic[50];
uniform float shapeRoughness[50];
uniform vec3 shapeAlbedos[50];

uniform int shapeCount;
uniform int shapeTypes[50];
uniform vec3 shapeCenters[50];
uniform vec4 shapeParams[50];
uniform vec3 shapeRotations[50];
uniform float shapeSmoothness[50];
uniform int shapeBlendOp[50];  

uniform int uSelectedCount;
uniform int uSelectedTypes[50];
uniform vec3 uSelectedCenters[50];
uniform vec4 uSelectedParams[50];
uniform vec3 uSelectedRotations[50];
uniform float uOutlineThickness;

uniform vec3 cameraPos;
uniform vec3 cameraTarget;

uniform int uRenderMode;

struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
};
struct SDFResult {
    float dist;
    Material mat;
};

const int MAX_STEPS = 100;
const float MAX_DIST = 100.0;
const float SURFACE_DIST = 0.001;

// Number of rays propagated per pixel
#define N_SAMPLES 1

// Maximum number of bounces per ray
#define N_BOUNCES 8

// Increase to avoid visual artifacts if SDFs get excessively non-Euclidean
#define SAFETY_FACTOR 2.

// Max number of ray marching steps
#define MAX_STEPS 250 

// Minimum distance from any scene surface below which ray marching stops
#define MIN_DIST 5e-4

// Maximum distance from any scene surface beyond which ray marching stops
#define MAX_DIST 100.




SDFResult opSmoothUnion(SDFResult a, SDFResult b, float k)
{
    float h = clamp(0.5 + 0.5*(b.dist - a.dist)/k, 0.0, 1.0);
    SDFResult res;
    res.dist = mix(b.dist, a.dist, h) - k * h * (1.0 - h);
    res.mat.albedo = mix(b.mat.albedo, a.mat.albedo, h);
    res.mat.metallic = mix(b.mat.metallic, a.mat.metallic, h);
    res.mat.roughness = mix(b.mat.roughness, a.mat.roughness, h);
    return res;
}
SDFResult opSmoothSubtraction(SDFResult a, SDFResult b, float k)
{
    float h = clamp(0.5 - 0.5*(b.dist + a.dist)/k, 0.0, 1.0);
    SDFResult res;
    res.dist = mix(b.dist, -a.dist, h) + k * h * (1.0 - h);
    res.mat.albedo = mix(b.mat.albedo, a.mat.albedo, h);
    res.mat.metallic = mix(b.mat.metallic, a.mat.metallic, h);
    res.mat.roughness = mix(b.mat.roughness, a.mat.roughness, h);
    return res;
}
SDFResult opSmoothIntersection(SDFResult a, SDFResult b, float k)
{
    float h = clamp(0.5 - 0.5*(b.dist - a.dist)/k, 0.0, 1.0);
    SDFResult res;
    res.dist = mix(b.dist, a.dist, h) + k * h * (1.0 - h);
    res.mat.albedo = mix(b.mat.albedo, a.mat.albedo, h);
    res.mat.metallic = mix(b.mat.metallic, a.mat.metallic, h);
    res.mat.roughness = mix(b.mat.roughness, a.mat.roughness, h);
    return res;
}

vec3 rotate(vec3 p, vec3 angles) {
    float cx = cos(angles.x), sx = sin(angles.x);
    float cy = cos(angles.y), sy = sin(angles.y);
    float cz = cos(angles.z), sz = sin(angles.z);
    vec3 rx = vec3(p.x, cx * p.y - sx * p.z, sx * p.y + cx * p.z);
    vec3 ry = vec3(cy * rx.x + sy * rx.z, rx.y, -sy * rx.x + cy * rx.z);
    vec3 rz = vec3(cz * ry.x - sz * ry.y, sz * ry.x + cz * ry.y, ry.z);
    return rz;
}

float sdSphere(vec3 p, vec3 center, float radius) {
    return length(p - center) - radius;
}
float sdBoxRotated(vec3 p, vec3 halfExtents, vec3 rotation) {
    vec3 localP = rotate(p, -rotation);
    vec3 d = abs(localP) - halfExtents;
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}
float sdRoundBox(vec3 p, vec3 b, float r) {
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) - r + min(max(q.x, max(q.y, q.z)), 0.0);
}
float sdRoundBoxRotated(vec3 p, vec3 b, float r, vec3 rotation) {
    vec3 localP = rotate(p, -rotation);
    return sdRoundBox(localP, b, r);
}
float sdTorus(vec3 p, vec2 t) {
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}
float sdTorusRotated(vec3 p, vec2 t, vec3 rotation) {
    vec3 localP = rotate(p, -rotation);
    return sdTorus(localP, t);
}
float sdCylinder(vec3 p, float r, float h) {
    vec2 d = vec2(length(p.xz) - r, abs(p.y) - h);
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}
float sdCylinderRotated(vec3 p, float r, float h, vec3 rotation) {
    vec3 localP = rotate(p, -rotation);
    return sdCylinder(localP, r, h);
}

SDFResult sdfForShape(int i, vec3 p) {
    SDFResult res;
    float d = MAX_DIST;
    if (shapeTypes[i] == 0) { // Sphere
         d = sdSphere(p, shapeCenters[i], shapeParams[i].w);
    } else if (shapeTypes[i] == 1) { // Box
         d = sdBoxRotated(p - shapeCenters[i], shapeParams[i].xyz, shapeRotations[i]);
    } else if (shapeTypes[i] == 2) { // Round Box
         d = sdRoundBoxRotated(p - shapeCenters[i], shapeParams[i].xyz, shapeParams[i].w, shapeRotations[i]);
    } else if (shapeTypes[i] == 3) { // Torus
         d = sdTorusRotated(p - shapeCenters[i], vec2(shapeParams[i].x, shapeParams[i].y), shapeRotations[i]);
    } else if (shapeTypes[i] == 4) { // Cylinder
         d = sdCylinderRotated(p - shapeCenters[i], shapeParams[i].x, shapeParams[i].y, shapeRotations[i]);
    }
    res.dist = d;
    res.mat.albedo = shapeAlbedos[i];
    res.mat.metallic = shapeMetallic[i];
    res.mat.roughness = shapeRoughness[i];
    return res;
}

SDFResult sceneSDF(vec3 p) {
    SDFResult res;
    res.dist = MAX_DIST;
    res.mat.albedo = vec3(0.0);
    for (int i = 0; i < shapeCount; i++) {
         SDFResult candidate = sdfForShape(i, p);
         if (shapeBlendOp[i] == 1) {
              res = opSmoothUnion(candidate, res, shapeSmoothness[i]);
         } else if (shapeBlendOp[i] == 2) {
              res = opSmoothSubtraction(candidate, res, shapeSmoothness[i]);
         } else if (shapeBlendOp[i] == 3) {
              res = opSmoothIntersection(candidate, res, shapeSmoothness[i]);
         } else {
              if (candidate.dist < res.dist)
                  res = candidate;
         }
    }
    return res;
}

vec3 getNormal(vec3 p) {
    float eps = 0.001;
    return normalize(vec3(
       sceneSDF(p + vec3(eps, 0, 0)).dist - sceneSDF(p - vec3(eps, 0, 0)).dist,
       sceneSDF(p + vec3(0, eps, 0)).dist - sceneSDF(p - vec3(0, eps, 0)).dist,
       sceneSDF(p + vec3(0, 0, eps)).dist - sceneSDF(p - vec3(0, 0, eps)).dist
    ));
}