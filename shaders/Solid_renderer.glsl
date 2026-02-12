#version 330 core

out vec4 FragColor;
in vec2 fragCoord;

uniform vec2 iResolution;
uniform float iTime;

uniform samplerBuffer uShapeBuffer;
uniform int uShapeTexelStride;
uniform int shapeCount;
uniform samplerBuffer uAccelCellRanges;
uniform samplerBuffer uAccelShapeIndices;
uniform int uAccelGridEnabled;
uniform ivec3 uAccelGridDim;
uniform vec3 uAccelGridMin;
uniform vec3 uAccelGridInvCellSize;

uniform int uSelectedCount;
uniform int uSelectedIndices[128];
uniform float uOutlineThickness;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec3 backgroundColor;
uniform float ambientIntensity;
uniform float directLightIntensity;
uniform int uRenderMode;

uniform vec3 cameraPos;
uniform vec3 cameraTarget;
uniform float uCameraFovDegrees;
uniform int uCameraProjectionMode;
uniform float uCameraOrthoScale;
uniform int uBackgroundGradient;
uniform vec3 uSceneBoundsCenter;
uniform float uSceneBoundsRadius;

uniform int uShowAxisGuides;
uniform int uActiveAxis;
uniform vec3 uGuideCenter;
uniform vec3 uGuideAxisDirection;
uniform int uMirrorHelperShow;
uniform vec3 uMirrorHelperAxisEnabled;
uniform vec3 uMirrorHelperOffset;
uniform vec3 uMirrorHelperCenter;
uniform float uMirrorHelperExtent;
uniform int uMirrorHelperSelected;
uniform int uMirrorHelperSelectedAxis;

const int FLAG_HAS_ROTATION = 1 << 0;
const int FLAG_HAS_SCALE = 1 << 1;
const int FLAG_HAS_ELONGATION = 1 << 2;
const int FLAG_HAS_ROUNDING = 1 << 3;
const int FLAG_HAS_TWIST = 1 << 4;
const int FLAG_HAS_BEND = 1 << 5;
const int FLAG_HAS_MIRROR = 1 << 7;

const int BLEND_NONE = 0;
const int BLEND_SMOOTH_UNION = 1;
const int BLEND_SMOOTH_SUBTRACTION = 2;
const int BLEND_SMOOTH_INTERSECTION = 3;

const int MAX_STEPS = 64;
const float MAX_DIST = 100.0;
const float SURFACE_DIST = 0.001;
const float ACCEL_EXACT_REFINE_DIST = 0.004;
const float ACCEL_EXACT_REFINE_SHADOW_DIST = 0.004;
const float ACCEL_MIN_CELL_EXIT_REFINE = 0.002;
const float PI = 3.14159265359;

struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
    vec3 emission;
    float emissionStrength;
    float transmission;
    float ior;
};

struct SDFResult {
    float dist;
    Material mat;
};

struct ShapeData {
    int type;
    int blendOp;
    int flags;
    vec3 center;
    float influenceRadius;
    vec4 param;
    vec3 invRotRow0;
    vec3 invRotRow1;
    vec3 invRotRow2;
    vec3 scale;
    vec3 elongation;
    float roundness;
    vec3 twist;
    vec3 bend;
    vec3 mirrorAxis;
    vec3 mirrorOffset;
    float mirrorSmoothness;
    float smoothness;
    vec3 albedo;
    float metallic;
    float roughness;
    vec3 emission;
    float emissionStrength;
    float transmission;
    float ior;
};

vec4 fetchShapeTexel(int shapeIndex, int slot) {
    return texelFetch(uShapeBuffer, shapeIndex * uShapeTexelStride + slot);
}

vec4 fetchAccelCellRangeTexel(int cellIndex) {
    return texelFetch(uAccelCellRanges, cellIndex);
}

int fetchAccelShapeIndex(int accelIndex) {
    return int(texelFetch(uAccelShapeIndices, accelIndex).x + 0.5);
}

bool getAccelCellRange(vec3 p, out int start, out int count, out bool forceFullScan) {
    start = 0;
    count = 0;
    forceFullScan = false;

    if (uAccelGridEnabled == 0) {
        return false;
    }

    vec3 cellCoordF = floor((p - uAccelGridMin) * uAccelGridInvCellSize);
    if (cellCoordF.x < 0.0 || cellCoordF.y < 0.0 || cellCoordF.z < 0.0) {
        return false;
    }
    if (cellCoordF.x >= float(uAccelGridDim.x) ||
        cellCoordF.y >= float(uAccelGridDim.y) ||
        cellCoordF.z >= float(uAccelGridDim.z)) {
        return false;
    }

    ivec3 cell = ivec3(cellCoordF);
    int cellIndex = cell.x +
                    cell.y * uAccelGridDim.x +
                    cell.z * uAccelGridDim.x * uAccelGridDim.y;

    vec4 range = fetchAccelCellRangeTexel(cellIndex);
    start = int(range.x + 0.5);
    count = int(range.y + 0.5);
    forceFullScan = (range.z > 0.5);
    return true;
}

bool getAccelCellData(vec3 p, out ivec3 cell, out int start, out int count, out bool forceFullScan) {
    cell = ivec3(0);
    start = 0;
    count = 0;
    forceFullScan = false;

    if (uAccelGridEnabled == 0) {
        return false;
    }

    vec3 cellCoordF = floor((p - uAccelGridMin) * uAccelGridInvCellSize);
    if (cellCoordF.x < 0.0 || cellCoordF.y < 0.0 || cellCoordF.z < 0.0) {
        return false;
    }
    if (cellCoordF.x >= float(uAccelGridDim.x) ||
        cellCoordF.y >= float(uAccelGridDim.y) ||
        cellCoordF.z >= float(uAccelGridDim.z)) {
        return false;
    }

    cell = ivec3(cellCoordF);
    int cellIndex = cell.x +
                    cell.y * uAccelGridDim.x +
                    cell.z * uAccelGridDim.x * uAccelGridDim.y;
    vec4 range = fetchAccelCellRangeTexel(cellIndex);
    start = int(range.x + 0.5);
    count = int(range.y + 0.5);
    forceFullScan = (range.z > 0.5);
    return true;
}

float accelCellExitDistance(vec3 p, vec3 rd, ivec3 cell) {
    vec3 cellMin = uAccelGridMin + vec3(cell) / uAccelGridInvCellSize;
    vec3 cellMax = uAccelGridMin + vec3(cell + ivec3(1)) / uAccelGridInvCellSize;

    const float huge = 1e20;
    float tx = huge;
    float ty = huge;
    float tz = huge;

    if (rd.x > 1e-6) {
        tx = (cellMax.x - p.x) / rd.x;
    } else if (rd.x < -1e-6) {
        tx = (cellMin.x - p.x) / rd.x;
    }

    if (rd.y > 1e-6) {
        ty = (cellMax.y - p.y) / rd.y;
    } else if (rd.y < -1e-6) {
        ty = (cellMin.y - p.y) / rd.y;
    }

    if (rd.z > 1e-6) {
        tz = (cellMax.z - p.z) / rd.z;
    } else if (rd.z < -1e-6) {
        tz = (cellMin.z - p.z) / rd.z;
    }

    return max(min(tx, min(ty, tz)), 0.0);
}

float accumulateSceneDistanceLowerBoundForShape(vec3 p, int shapeIndex, float dist);
float accumulateSceneDistanceExactForShape(vec3 p, int shapeIndex, float dist);
SDFResult accumulateSceneFullForShape(vec3 p, int shapeIndex, SDFResult res);

float sceneSDFDistanceExactForAccelRange(vec3 p, int accelStart, int accelCount) {
    float dist = MAX_DIST;
    for (int j = 0; j < accelCount; ++j) {
        dist = accumulateSceneDistanceExactForShape(p, fetchAccelShapeIndex(accelStart + j), dist);
    }
    return dist;
}

float sceneSDFDistanceLowerBoundForAccelRange(vec3 p, int accelStart, int accelCount) {
    float dist = MAX_DIST;
    for (int j = 0; j < accelCount; ++j) {
        dist = accumulateSceneDistanceLowerBoundForShape(p, fetchAccelShapeIndex(accelStart + j), dist);
    }
    return dist;
}

SDFResult sceneSDFFullForAccelRange(vec3 p, int accelStart, int accelCount) {
    SDFResult res;
    res.dist = MAX_DIST;
    res.mat.albedo = vec3(0.0);
    res.mat.metallic = 0.0;
    res.mat.roughness = 1.0;
    res.mat.emission = vec3(0.0);
    res.mat.emissionStrength = 0.0;
    res.mat.transmission = 0.0;
    res.mat.ior = 1.5;
    for (int j = 0; j < accelCount; ++j) {
        res = accumulateSceneFullForShape(p, fetchAccelShapeIndex(accelStart + j), res);
    }
    return res;
}

bool hasFlag(int flags, int bit) {
    return (flags & bit) != 0;
}

void loadShapeHeader(int shapeIndex,
                     out vec4 t0,
                     out vec4 t1,
                     out int blendOp,
                     out vec3 center,
                     out float influenceRadius,
                     out float smoothness) {
    t0 = fetchShapeTexel(shapeIndex, 0);
    t1 = fetchShapeTexel(shapeIndex, 1);

    blendOp = int(t0.y + 0.5);
    center = t1.xyz;
    influenceRadius = t1.w;
    smoothness = max(t0.w, 1e-4);
}

ShapeData loadShapeCoreDataFromHeader(int shapeIndex, vec4 t0, vec4 t1) {
    ShapeData shape;
    shape.type = int(t0.x + 0.5);
    shape.blendOp = int(t0.y + 0.5);
    shape.flags = int(t0.z + 0.5);
    shape.smoothness = max(t0.w, 1e-4);

    shape.center = t1.xyz;
    shape.influenceRadius = t1.w;
    shape.param = fetchShapeTexel(shapeIndex, 2);

    shape.invRotRow0 = vec3(1.0, 0.0, 0.0);
    shape.invRotRow1 = vec3(0.0, 1.0, 0.0);
    shape.invRotRow2 = vec3(0.0, 0.0, 1.0);
    shape.scale = vec3(1.0);
    shape.elongation = vec3(0.0);
    shape.roundness = 0.0;
    shape.twist = vec3(0.0);
    shape.bend = vec3(0.0);
    shape.mirrorAxis = vec3(0.0);
    shape.mirrorOffset = vec3(0.0);
    shape.mirrorSmoothness = 0.0;

    if (hasFlag(shape.flags, FLAG_HAS_ROTATION)) {
        vec4 t3 = fetchShapeTexel(shapeIndex, 3);
        vec4 t4 = fetchShapeTexel(shapeIndex, 4);
        vec4 t5 = fetchShapeTexel(shapeIndex, 5);
        shape.invRotRow0 = t3.xyz;
        shape.invRotRow1 = t4.xyz;
        shape.invRotRow2 = t5.xyz;
        if (hasFlag(shape.flags, FLAG_HAS_ROUNDING)) {
            shape.roundness = max(t4.w, 0.0);
        }
    } else if (hasFlag(shape.flags, FLAG_HAS_ROUNDING)) {
        vec4 t4 = fetchShapeTexel(shapeIndex, 4);
        shape.roundness = max(t4.w, 0.0);
    }

    if (hasFlag(shape.flags, FLAG_HAS_SCALE)) {
        vec4 t6 = fetchShapeTexel(shapeIndex, 6);
        shape.scale = t6.xyz;
    }
    if (hasFlag(shape.flags, FLAG_HAS_ELONGATION)) {
        vec4 t7 = fetchShapeTexel(shapeIndex, 7);
        shape.elongation = t7.xyz;
    }
    if (hasFlag(shape.flags, FLAG_HAS_TWIST)) {
        vec4 t8 = fetchShapeTexel(shapeIndex, 8);
        shape.twist = t8.xyz;
    }
    if (hasFlag(shape.flags, FLAG_HAS_BEND)) {
        vec4 t9 = fetchShapeTexel(shapeIndex, 9);
        shape.bend = t9.xyz;
    }
    if (hasFlag(shape.flags, FLAG_HAS_MIRROR)) {
        vec4 t6 = fetchShapeTexel(shapeIndex, 6);
        vec4 t7 = fetchShapeTexel(shapeIndex, 7);
        vec4 t8 = fetchShapeTexel(shapeIndex, 8);
        vec4 t9 = fetchShapeTexel(shapeIndex, 9);
        vec4 t11 = fetchShapeTexel(shapeIndex, 11);
        shape.mirrorAxis = clamp(t11.yzw, vec3(0.0), vec3(1.0));
        shape.mirrorOffset = vec3(t6.w, t7.w, t8.w);
        shape.mirrorSmoothness = max(t9.w, 0.0);
    }

    return shape;
}

ShapeData loadShapeDataFromHeader(int shapeIndex, vec4 t0, vec4 t1) {
    ShapeData shape = loadShapeCoreDataFromHeader(shapeIndex, t0, t1);
    vec4 t10 = fetchShapeTexel(shapeIndex, 10);
    vec4 t11 = fetchShapeTexel(shapeIndex, 11);
    vec4 t12 = fetchShapeTexel(shapeIndex, 12);
    vec4 t13 = fetchShapeTexel(shapeIndex, 13);
    shape.albedo = t10.xyz;
    shape.metallic = t10.w;
    shape.roughness = t11.x;
    shape.emission = t12.xyz;
    shape.emissionStrength = max(t12.w, 0.0);
    shape.transmission = clamp(t13.x, 0.0, 1.0);
    shape.ior = max(t13.y, 1.0);
    return shape;
}

ShapeData loadShapeDistanceDataFromHeader(int shapeIndex, vec4 t0, vec4 t1) {
    ShapeData shape = loadShapeCoreDataFromHeader(shapeIndex, t0, t1);
    shape.albedo = vec3(0.0);
    shape.metallic = 0.0;
    shape.roughness = 1.0;
    shape.emission = vec3(0.0);
    shape.emissionStrength = 0.0;
    shape.transmission = 0.0;
    shape.ior = 1.5;
    return shape;
}

SDFResult opSmoothUnion(SDFResult candidate, SDFResult scene, float smoothness) {
    float k = max(smoothness, 1e-4);
    float h = clamp(0.5 + 0.5 * (scene.dist - candidate.dist) / k, 0.0, 1.0);
    SDFResult res;
    res.dist = mix(scene.dist, candidate.dist, h) - k * h * (1.0 - h);
    res.mat.albedo = mix(scene.mat.albedo, candidate.mat.albedo, h);
    res.mat.metallic = mix(scene.mat.metallic, candidate.mat.metallic, h);
    res.mat.roughness = mix(scene.mat.roughness, candidate.mat.roughness, h);
    res.mat.emission = mix(scene.mat.emission, candidate.mat.emission, h);
    res.mat.emissionStrength = mix(scene.mat.emissionStrength, candidate.mat.emissionStrength, h);
    res.mat.transmission = mix(scene.mat.transmission, candidate.mat.transmission, h);
    res.mat.ior = mix(scene.mat.ior, candidate.mat.ior, h);
    return res;
}

SDFResult opSmoothSubtraction(SDFResult candidate, SDFResult scene, float smoothness) {
    float k = max(smoothness, 1e-4);
    float h = clamp(0.5 - 0.5 * (scene.dist + candidate.dist) / k, 0.0, 1.0);
    SDFResult res;
    res.dist = mix(scene.dist, -candidate.dist, h) + k * h * (1.0 - h);
    res.mat.albedo = mix(scene.mat.albedo, candidate.mat.albedo, h);
    res.mat.metallic = mix(scene.mat.metallic, candidate.mat.metallic, h);
    res.mat.roughness = mix(scene.mat.roughness, candidate.mat.roughness, h);
    res.mat.emission = mix(scene.mat.emission, candidate.mat.emission, h);
    res.mat.emissionStrength = mix(scene.mat.emissionStrength, candidate.mat.emissionStrength, h);
    res.mat.transmission = mix(scene.mat.transmission, candidate.mat.transmission, h);
    res.mat.ior = mix(scene.mat.ior, candidate.mat.ior, h);
    return res;
}

SDFResult opSmoothIntersection(SDFResult candidate, SDFResult scene, float smoothness) {
    float k = max(smoothness, 1e-4);
    float h = clamp(0.5 - 0.5 * (scene.dist - candidate.dist) / k, 0.0, 1.0);
    SDFResult res;
    res.dist = mix(scene.dist, candidate.dist, h) + k * h * (1.0 - h);
    res.mat.albedo = mix(scene.mat.albedo, candidate.mat.albedo, h);
    res.mat.metallic = mix(scene.mat.metallic, candidate.mat.metallic, h);
    res.mat.roughness = mix(scene.mat.roughness, candidate.mat.roughness, h);
    res.mat.emission = mix(scene.mat.emission, candidate.mat.emission, h);
    res.mat.emissionStrength = mix(scene.mat.emissionStrength, candidate.mat.emissionStrength, h);
    res.mat.transmission = mix(scene.mat.transmission, candidate.mat.transmission, h);
    res.mat.ior = mix(scene.mat.ior, candidate.mat.ior, h);
    return res;
}

float opSmoothUnionDistance(float candidateDist, float sceneDist, float smoothness) {
    float k = max(smoothness, 1e-4);
    float h = clamp(0.5 + 0.5 * (sceneDist - candidateDist) / k, 0.0, 1.0);
    return mix(sceneDist, candidateDist, h) - k * h * (1.0 - h);
}

float opSmoothSubtractionDistance(float candidateDist, float sceneDist, float smoothness) {
    float k = max(smoothness, 1e-4);
    float h = clamp(0.5 - 0.5 * (sceneDist + candidateDist) / k, 0.0, 1.0);
    return mix(sceneDist, -candidateDist, h) + k * h * (1.0 - h);
}

float opSmoothIntersectionDistance(float candidateDist, float sceneDist, float smoothness) {
    float k = max(smoothness, 1e-4);
    float h = clamp(0.5 - 0.5 * (sceneDist - candidateDist) / k, 0.0, 1.0);
    return mix(sceneDist, candidateDist, h) + k * h * (1.0 - h);
}

vec3 safeScale(vec3 s) {
    return max(abs(s), vec3(0.001));
}

vec3 opTwistPoint(vec3 p, float k, int axis) {
    if (abs(k) < 1e-6) return p;
    int ax = clamp(axis, 0, 2);
    if (ax == 0) {
        float c = cos(k * p.x);
        float s = sin(k * p.x);
        mat2 m = mat2(c, -s, s, c);
        vec2 yz = m * p.yz;
        return vec3(p.x, yz.x, yz.y);
    } else if (ax == 1) {
        float c = cos(k * p.y);
        float s = sin(k * p.y);
        mat2 m = mat2(c, -s, s, c);
        vec2 xz = m * p.xz;
        return vec3(xz.x, p.y, xz.y);
    }

    float c = cos(k * p.z);
    float s = sin(k * p.z);
    mat2 m = mat2(c, -s, s, c);
    vec2 xy = m * p.xy;
    return vec3(xy.x, xy.y, p.z);
}

vec3 opCheapBendPoint(vec3 p, float k, int axis) {
    if (abs(k) < 1e-6) return p;
    int ax = clamp(axis, 0, 2);
    if (ax == 0) {
        float c = cos(k * p.y);
        float s = sin(k * p.y);
        mat2 m = mat2(c, -s, s, c);
        vec2 yz = m * p.yz;
        return vec3(p.x, yz.x, yz.y);
    } else if (ax == 1) {
        float c = cos(k * p.z);
        float s = sin(k * p.z);
        mat2 m = mat2(c, -s, s, c);
        vec2 xz = m * p.xz;
        return vec3(xz.x, p.y, xz.y);
    }

    float c = cos(k * p.x);
    float s = sin(k * p.x);
    mat2 m = mat2(c, -s, s, c);
    vec2 xy = m * p.xy;
    return vec3(xy.x, xy.y, p.z);
}

vec3 applyTwistCombined(vec3 p, vec3 k) {
    vec3 result = p;
    result = opTwistPoint(result, k.x, 0);
    result = opTwistPoint(result, k.y, 1);
    result = opTwistPoint(result, k.z, 2);
    return result;
}

vec3 applyBendCombined(vec3 p, vec3 k) {
    vec3 result = p;
    result = opCheapBendPoint(result, k.x, 0);
    result = opCheapBendPoint(result, k.y, 1);
    result = opCheapBendPoint(result, k.z, 2);
    return result;
}

float sdSphereLocal(vec3 p, float radius) {
    return length(p) - radius;
}

float sdBoxLocal(vec3 p, vec3 halfExtents) {
    vec3 d = abs(p) - halfExtents;
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

float sdTorusLocal(vec3 p, vec2 t) {
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}

float sdCylinderLocal(vec3 p, float r, float h) {
    vec2 d = vec2(length(p.xz) - r, abs(p.y) - h);
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float sdConeLocal(vec3 p, float baseRadius, float halfHeight) {
    vec2 q = vec2(length(p.xz), p.y);
    vec2 k1 = vec2(0.0, halfHeight);
    vec2 k2 = vec2(-baseRadius, 2.0 * halfHeight);
    vec2 ca = vec2(q.x - min(q.x, (q.y < 0.0) ? baseRadius : 0.0), abs(q.y) - halfHeight);
    vec2 cb = q - k1 + k2 * clamp(dot(k1 - q, k2) / max(dot(k2, k2), 1e-6), 0.0, 1.0);
    float s = (cb.x < 0.0 && ca.y < 0.0) ? -1.0 : 1.0;
    return s * sqrt(max(min(dot(ca, ca), dot(cb, cb)), 0.0));
}

float sdMandelbulbLocal(vec3 p, float power, float iterationsF, float bailout) {
    float safePower = clamp(power, 2.0, 16.0);
    int iterations = int(clamp(floor(iterationsF + 0.5), 1.0, 64.0));
    float bailoutRadius = max(bailout, 2.0);

    vec3 z = p;
    float dr = 1.0;
    float r = 0.0;

    for (int i = 0; i < 64; ++i) {
        if (i >= iterations) {
            break;
        }

        r = length(z);
        if (r > bailoutRadius) {
            break;
        }

        float safeR = max(r, 1e-6);
        float theta = acos(clamp(z.z / safeR, -1.0, 1.0));
        float phi = atan(z.y, z.x);
        float zr = pow(safeR, safePower);
        dr = pow(safeR, safePower - 1.0) * safePower * dr + 1.0;

        float t = theta * safePower;
        float pAngle = phi * safePower;
        float sinT = sin(t);
        z = zr * vec3(sinT * cos(pAngle), sinT * sin(pAngle), cos(t)) + p;
    }

    r = max(r, 1e-6);
    return 0.5 * log(r) * r / max(dr, 1e-6);
}

float sdPrimitiveLocal(int type, vec3 p, vec4 param) {
    if (type == 0) {
        return sdSphereLocal(p, param.w);
    } else if (type == 1) {
        return sdBoxLocal(p, param.xyz);
    } else if (type == 2) {
        return sdTorusLocal(p, vec2(param.x, param.y));
    } else if (type == 3) {
        return sdCylinderLocal(p, param.x, param.y);
    } else if (type == 4) {
        return sdConeLocal(p, param.x, param.y);
    } else if (type == 5) {
        return sdMandelbulbLocal(p, param.x, param.y, param.z);
    }
    return MAX_DIST;
}

float applyDeformScale(int type, vec3 localP, vec4 param, vec3 scale) {
    vec3 s = safeScale(scale);
    float k = min(min(s.x, s.y), s.z);
    return sdPrimitiveLocal(type, localP / s, param) * k;
}

float evaluateShapeDistanceSingle(ShapeData shape, vec3 p) {
    vec3 localP = p - shape.center;

    if (hasFlag(shape.flags, FLAG_HAS_ROTATION)) {
        localP = vec3(
            dot(shape.invRotRow0, localP),
            dot(shape.invRotRow1, localP),
            dot(shape.invRotRow2, localP)
        );
    }

    if (hasFlag(shape.flags, FLAG_HAS_TWIST)) {
        localP = applyTwistCombined(localP, shape.twist);
    }

    if (hasFlag(shape.flags, FLAG_HAS_BEND)) {
        localP = applyBendCombined(localP, shape.bend);
    }

    vec3 q = localP;
    float correction = 0.0;
    if (hasFlag(shape.flags, FLAG_HAS_ELONGATION)) {
        vec3 raw = abs(localP) - shape.elongation;
        q = max(raw, 0.0);
        correction = min(max(raw.x, max(raw.y, raw.z)), 0.0);
    }

    float d = 0.0;
    if (hasFlag(shape.flags, FLAG_HAS_SCALE)) {
        d = applyDeformScale(shape.type, q, shape.param, shape.scale) + correction;
    } else {
        d = sdPrimitiveLocal(shape.type, q, shape.param) + correction;
    }

    if (hasFlag(shape.flags, FLAG_HAS_ROUNDING)) {
        d -= max(shape.roundness, 0.0);
    }

    if (hasFlag(shape.flags, FLAG_HAS_TWIST) || hasFlag(shape.flags, FLAG_HAS_BEND)) {
        float warpAmount = dot(abs(shape.twist), vec3(1.0)) + dot(abs(shape.bend), vec3(1.0));
        float safety = 1.0 + 0.35 * min(warpAmount, 8.0);
        d /= safety;
    }

    return d;
}

float evaluateShapeDistance(ShapeData shape, vec3 p) {
    float d = evaluateShapeDistanceSingle(shape, p);
    if (!hasFlag(shape.flags, FLAG_HAS_MIRROR)) {
        return d;
    }

    int maxX = (shape.mirrorAxis.x > 0.5) ? 1 : 0;
    int maxY = (shape.mirrorAxis.y > 0.5) ? 1 : 0;
    int maxZ = (shape.mirrorAxis.z > 0.5) ? 1 : 0;
    if (maxX == 0 && maxY == 0 && maxZ == 0) {
        return d;
    }
    float mirrorK = max(shape.mirrorSmoothness, 0.0);

    for (int ix = 0; ix <= maxX; ++ix) {
        for (int iy = 0; iy <= maxY; ++iy) {
            for (int iz = 0; iz <= maxZ; ++iz) {
                if (ix == 0 && iy == 0 && iz == 0) {
                    continue;
                }

                vec3 reflectedP = p;
                if (ix == 1) reflectedP.x = 2.0 * shape.mirrorOffset.x - reflectedP.x;
                if (iy == 1) reflectedP.y = 2.0 * shape.mirrorOffset.y - reflectedP.y;
                if (iz == 1) reflectedP.z = 2.0 * shape.mirrorOffset.z - reflectedP.z;
                float reflectedD = evaluateShapeDistanceSingle(shape, reflectedP);
                if (mirrorK > 1e-6) {
                    d = opSmoothUnionDistance(reflectedD, d, mirrorK);
                } else {
                    d = min(d, reflectedD);
                }
            }
        }
    }

    return d;
}

SDFResult sdfForShape(ShapeData shape, vec3 p) {
    SDFResult res;
    res.dist = evaluateShapeDistance(shape, p);
    res.mat.albedo = shape.albedo;
    res.mat.metallic = shape.metallic;
    res.mat.roughness = shape.roughness;
    res.mat.emission = shape.emission;
    res.mat.emissionStrength = shape.emissionStrength;
    res.mat.transmission = shape.transmission;
    res.mat.ior = shape.ior;
    return res;
}

bool distanceGE(float centerDistSq, float threshold) {
    if (threshold <= 0.0) {
        return true;
    }
    return centerDistSq >= threshold * threshold;
}

bool distanceLE(float centerDistSq, float threshold) {
    if (threshold < 0.0) {
        return false;
    }
    return centerDistSq <= threshold * threshold;
}

bool canSkipShape(int blendOp, float centerDistSq, float influenceRadius, float currentDist, float smoothness) {
    float k = max(smoothness, 1e-4);
    if (blendOp == BLEND_NONE) {
        return distanceGE(centerDistSq, currentDist + influenceRadius);
    }
    if (blendOp == BLEND_SMOOTH_UNION) {
        return distanceGE(centerDistSq, currentDist + k + influenceRadius);
    }
    if (blendOp == BLEND_SMOOTH_SUBTRACTION) {
        return distanceGE(centerDistSq, k - currentDist + influenceRadius);
    }
    if (blendOp == BLEND_SMOOTH_INTERSECTION) {
        return distanceLE(centerDistSq, currentDist - k - influenceRadius);
    }
    return distanceGE(centerDistSq, currentDist + influenceRadius);
}

float accumulateSceneDistanceLowerBoundForShape(vec3 p, int shapeIndex, float dist) {
    vec4 t0;
    vec4 t1;
    int blendOp;
    vec3 center;
    float influenceRadius;
    float smoothness;
    loadShapeHeader(shapeIndex, t0, t1, blendOp, center, influenceRadius, smoothness);

    if (blendOp == BLEND_SMOOTH_SUBTRACTION || blendOp == BLEND_SMOOTH_INTERSECTION) {
        return dist;
    }

    vec3 centerDelta = p - center;
    float centerDistSq = dot(centerDelta, centerDelta);
    if (canSkipShape(blendOp, centerDistSq, influenceRadius, dist, smoothness)) {
        return dist;
    }

    float lb = sqrt(centerDistSq) - influenceRadius;
    if (blendOp == BLEND_SMOOTH_UNION) {
        return min(dist, lb - 0.25 * smoothness);
    }
    return min(dist, lb);
}

float accumulateSceneDistanceExactForShape(vec3 p, int shapeIndex, float dist) {
    vec4 t0;
    vec4 t1;
    int blendOp;
    vec3 center;
    float influenceRadius;
    float smoothness;
    loadShapeHeader(shapeIndex, t0, t1, blendOp, center, influenceRadius, smoothness);

    vec3 centerDelta = p - center;
    float centerDistSq = dot(centerDelta, centerDelta);
    if (canSkipShape(blendOp, centerDistSq, influenceRadius, dist, smoothness)) {
        return dist;
    }

    ShapeData shape = loadShapeDistanceDataFromHeader(shapeIndex, t0, t1);
    float candidateDist = evaluateShapeDistance(shape, p);

    if (blendOp == BLEND_SMOOTH_UNION) {
        return opSmoothUnionDistance(candidateDist, dist, smoothness);
    }
    if (blendOp == BLEND_SMOOTH_SUBTRACTION) {
        return opSmoothSubtractionDistance(candidateDist, dist, smoothness);
    }
    if (blendOp == BLEND_SMOOTH_INTERSECTION) {
        return opSmoothIntersectionDistance(candidateDist, dist, smoothness);
    }
    return min(dist, candidateDist);
}

SDFResult accumulateSceneFullForShape(vec3 p, int shapeIndex, SDFResult res) {
    vec4 t0;
    vec4 t1;
    int blendOp;
    vec3 center;
    float influenceRadius;
    float smoothness;
    loadShapeHeader(shapeIndex, t0, t1, blendOp, center, influenceRadius, smoothness);

    vec3 centerDelta = p - center;
    float centerDistSq = dot(centerDelta, centerDelta);
    if (canSkipShape(blendOp, centerDistSq, influenceRadius, res.dist, smoothness)) {
        return res;
    }

    ShapeData shape = loadShapeDataFromHeader(shapeIndex, t0, t1);
    SDFResult candidate = sdfForShape(shape, p);

    if (blendOp == BLEND_SMOOTH_UNION) {
        return opSmoothUnion(candidate, res, smoothness);
    }
    if (blendOp == BLEND_SMOOTH_SUBTRACTION) {
        return opSmoothSubtraction(candidate, res, smoothness);
    }
    if (blendOp == BLEND_SMOOTH_INTERSECTION) {
        return opSmoothIntersection(candidate, res, smoothness);
    }
    if (candidate.dist < res.dist) {
        return candidate;
    }
    return res;
}

float sceneSDFDistanceLowerBound(vec3 p) {
    float dist = MAX_DIST;
    for (int i = 0; i < shapeCount; ++i) {
        dist = accumulateSceneDistanceLowerBoundForShape(p, i, dist);
    }

    return dist;
}

float sceneSDFDistanceExact(vec3 p) {
    float dist = MAX_DIST;
    for (int i = 0; i < shapeCount; ++i) {
        dist = accumulateSceneDistanceExactForShape(p, i, dist);
    }

    return dist;
}

SDFResult sceneSDFFull(vec3 p) {
    SDFResult res;
    res.dist = MAX_DIST;
    res.mat.albedo = vec3(0.0);
    res.mat.metallic = 0.0;
    res.mat.roughness = 1.0;
    res.mat.emission = vec3(0.0);
    res.mat.emissionStrength = 0.0;
    res.mat.transmission = 0.0;
    res.mat.ior = 1.5;
    for (int i = 0; i < shapeCount; ++i) {
        res = accumulateSceneFullForShape(p, i, res);
    }

    return res;
}

vec3 getNormal(vec3 p) {
    const float eps = 0.001;
    const vec3 k1 = vec3(1.0, -1.0, -1.0);
    const vec3 k2 = vec3(-1.0, -1.0, 1.0);
    const vec3 k3 = vec3(-1.0, 1.0, -1.0);
    const vec3 k4 = vec3(1.0, 1.0, 1.0);

    vec3 n =
        k1 * sceneSDFDistanceExact(p + k1 * eps) +
        k2 * sceneSDFDistanceExact(p + k2 * eps) +
        k3 * sceneSDFDistanceExact(p + k3 * eps) +
        k4 * sceneSDFDistanceExact(p + k4 * eps);
    return normalize(n);
}

bool intersectSceneBounds(vec3 ro, vec3 rd, out float tEnter, out float tExit) {
    if (uSceneBoundsRadius <= 0.0) {
        tEnter = 0.0;
        tExit = -1.0;
        return false;
    }

    vec3 oc = ro - uSceneBoundsCenter;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - uSceneBoundsRadius * uSceneBoundsRadius;
    float h = b * b - c;
    if (h < 0.0) {
        tEnter = 0.0;
        tExit = -1.0;
        return false;
    }

    float s = sqrt(h);
    tEnter = -b - s;
    tExit = -b + s;
    if (tExit < 0.0) {
        return false;
    }

    tEnter = max(tEnter, 0.0);
    return true;
}

float softShadow(vec3 ro, vec3 rd, float mint, float maxt, float k) {
    float boundsEnter;
    float boundsExit;
    if (!intersectSceneBounds(ro, rd, boundsEnter, boundsExit)) {
        return 1.0;
    }

    float res = 1.0;
    float t = max(mint, boundsEnter);
    float maxT = min(maxt, boundsExit);
    if (t >= maxT) {
        return 1.0;
    }

    for (int i = 0; i < 12; ++i) {
        vec3 samplePos = ro + rd * t;
        float h = MAX_DIST;

        ivec3 cell;
        int accelStart;
        int accelCount;
        bool forceFullScan;
        bool hasCell = getAccelCellData(samplePos, cell, accelStart, accelCount, forceFullScan);
        float cellExit = 0.0;

        if (hasCell && !forceFullScan) {
            cellExit = accelCellExitDistance(samplePos, rd, cell);
            if (accelCount > 0) {
                float hLower = sceneSDFDistanceLowerBoundForAccelRange(samplePos, accelStart, accelCount);
                if (hLower <= ACCEL_EXACT_REFINE_SHADOW_DIST || cellExit <= ACCEL_MIN_CELL_EXIT_REFINE) {
                    h = sceneSDFDistanceExact(samplePos);
                } else {
                    h = hLower;
                }
            } else {
                h = cellExit;
            }
        } else {
            h = sceneSDFDistanceExact(samplePos);
        }

        if (h < SURFACE_DIST * 0.5) {
            return 0.0;
        }
        float stepDist = max(h, SURFACE_DIST * 0.25);
        if (hasCell && !forceFullScan) {
            stepDist = min(stepDist, cellExit + SURFACE_DIST * 0.5);
        }
        res = min(res, k * stepDist / max(t, 0.001));
        t += stepDist;
        if (t >= maxT) {
            break;
        }
    }
    return res;
}

float selectedShapeSDFFor(int selectedIdx, vec3 p) {
    int shapeIndex = uSelectedIndices[selectedIdx];
    if (shapeIndex < 0 || shapeIndex >= shapeCount) {
        return MAX_DIST;
    }
    vec4 t0;
    vec4 t1;
    int blendOp;
    vec3 center;
    float influenceRadius;
    float smoothness;
    loadShapeHeader(shapeIndex, t0, t1, blendOp, center, influenceRadius, smoothness);
    ShapeData shape = loadShapeDistanceDataFromHeader(shapeIndex, t0, t1);
    return evaluateShapeDistance(shape, p);
}

vec4 rayMarchSelectedFor(int idx, vec3 ro, vec3 rd) {
    const float maxDist = MAX_DIST;
    const float minDist = SURFACE_DIST;
    const int maxIter = 40;

    float dist = 0.0;
    float lastD = 1e10;
    float edge = 0.0;

    for (int i = 0; i < maxIter; ++i) {
        vec3 pos = ro + rd * dist;
        float dEval = selectedShapeSDFFor(idx, pos);
        if (dEval < minDist) {
            break;
        }
        if (lastD < uOutlineThickness && dEval > lastD + 0.001) {
            edge = 1.0;
            break;
        }
        lastD = dEval;
        dist += dEval;
        if (dist > maxDist) {
            break;
        }
    }

    return vec4(dist, 0.0, edge, 0.0);
}

vec3 AgXToneMapping(vec3 color) {
    vec3 aces = clamp((color * (color + 0.0245786) - 0.000090537) /
                      (color * (0.983729 * color + 0.4329510) + 0.238081),
                      0.0, 1.0);
    return mix(aces, color, 0.35);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float distributionGGX(float NdotH, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0;
    return alpha2 / max(PI * denom * denom, 1e-6);
}

float geometrySchlickGGX(float NdotX, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotX / max(NdotX * (1.0 - k) + k, 1e-6);
}

float geometrySmith(float NdotV, float NdotL, float roughness) {
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

float sdAxisLine(vec3 p, vec3 center, vec3 axisDir) {
    vec3 localP = p - center;
    float dirLen = length(axisDir);
    if (dirLen < 1e-6) {
        return MAX_DIST;
    }

    vec3 dir = axisDir / dirLen;
    vec3 radial = localP - dir * dot(localP, dir);
    return length(radial);
}

vec4 rayMarchAxisGuide(vec3 ro, vec3 rd, vec3 center, vec3 axisDir) {
    const float maxDist = MAX_DIST;
    const float minDist = 0.002;
    const int maxIter = 64;

    float dist = 0.0;
    float lineWidth = 0.002;

    for (int i = 0; i < maxIter; ++i) {
        vec3 pos = ro + rd * dist;
        float d = sdAxisLine(pos, center, axisDir) - lineWidth;
        if (d < minDist) {
            return vec4(dist, 0.0, 1.0, 0.0);
        }
        dist += d;
        if (dist > maxDist) {
            break;
        }
    }

    return vec4(maxDist, 0.0, 0.0, 0.0);
}

void computeCameraRay(vec2 uv, out vec3 ro, out vec3 rd) {
    vec2 pScreen = (uv - 0.5) * 2.0;
    pScreen.x *= iResolution.x / max(iResolution.y, 1.0);

    vec3 forward = normalize(cameraTarget - cameraPos);
    vec3 right = normalize(cross(forward, vec3(0.0, 1.0, 0.0)));
    if (length(right) < 1e-6) {
        right = vec3(1.0, 0.0, 0.0);
    }
    vec3 up = normalize(cross(right, forward));

    if (uCameraProjectionMode == 1) {
        float orthoScale = max(uCameraOrthoScale, 1e-4);
        ro = cameraPos + (pScreen.x * right + pScreen.y * up) * orthoScale;
        rd = forward;
        return;
    }

    float tanHalfFov = tan(radians(clamp(uCameraFovDegrees, 1.0, 179.0)) * 0.5);
    vec2 perspectiveOffset = pScreen * tanHalfFov;
    ro = cameraPos;
    rd = normalize(forward + perspectiveOffset.x * right + perspectiveOffset.y * up);
}

void mainImage(out vec4 fragColor, in vec2 fragCoordIn) {
    vec2 uv = fragCoordIn * 0.5 + 0.5;
    vec3 ro;
    vec3 rd;
    computeCameraRay(uv, ro, rd);

    vec3 hitPos = ro;
    float dist = MAX_DIST;
    bool hitScene = false;

    float boundsEnter;
    float boundsExit;
    if (intersectSceneBounds(ro, rd, boundsEnter, boundsExit)) {
        float marchDist = boundsEnter;
        float marchEnd = min(MAX_DIST, boundsExit);
        for (int i = 0; i < MAX_STEPS; ++i) {
            hitPos = ro + rd * marchDist;
            float dScene = MAX_DIST;
            float cellExit = 0.0;

            ivec3 cell;
            int accelStart;
            int accelCount;
            bool forceFullScan;
            bool hasCell = getAccelCellData(hitPos, cell, accelStart, accelCount, forceFullScan);
            if (hasCell && !forceFullScan) {
                cellExit = accelCellExitDistance(hitPos, rd, cell);
                if (accelCount > 0) {
                    float dLower = sceneSDFDistanceLowerBoundForAccelRange(hitPos, accelStart, accelCount);
                    if (dLower <= ACCEL_EXACT_REFINE_DIST || cellExit <= ACCEL_MIN_CELL_EXIT_REFINE) {
                        dScene = sceneSDFDistanceExact(hitPos);
                    } else {
                        dScene = dLower;
                    }
                } else {
                    dScene = cellExit;
                }
            } else {
                dScene = sceneSDFDistanceExact(hitPos);
            }

            if (abs(dScene) < SURFACE_DIST) {
                hitScene = true;
                dist = marchDist;
                break;
            }
            float stepDist = max(dScene, SURFACE_DIST * 0.25);
            if (hasCell && !forceFullScan) {
                stepDist = min(stepDist, cellExit + SURFACE_DIST * 0.5);
            }
            marchDist += stepDist;
            if (marchDist > marchEnd) {
                break;
            }
        }
    }

    vec3 color = vec3(0.0);
    if (hitScene) {
        vec3 N = normalize(getNormal(hitPos));
        vec3 V = normalize(ro - hitPos);
        SDFResult hitRes = sceneSDFFull(hitPos);

        vec3 baseColor = max(hitRes.mat.albedo, vec3(0.0));
        float metallic = clamp(hitRes.mat.metallic, 0.0, 1.0);
        float roughness = clamp(hitRes.mat.roughness, 0.04, 1.0);
        vec3 emission = max(hitRes.mat.emission, vec3(0.0)) * max(hitRes.mat.emissionStrength, 0.0);
        float transmission = clamp(hitRes.mat.transmission, 0.0, 1.0);
        float ior = max(hitRes.mat.ior, 1.0);

        vec3 F0 = mix(vec3(0.04), baseColor, metallic);
        vec3 L = normalize(lightDir);
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        vec3 H = normalize(L + V);
        float NdotH = max(dot(N, H), 0.0);
        float HdotV = max(dot(H, V), 0.0);

        vec3 F = fresnelSchlick(HdotV, F0);
        float D = distributionGGX(NdotH, roughness);
        float G = geometrySmith(NdotV, NdotL, roughness);

        vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-4);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic) * (1.0 - transmission);
        vec3 diffuse = kD * baseColor / PI;

        float shadow = softShadow(hitPos + N * SURFACE_DIST * 2.0, L, 0.001, MAX_DIST, 32.0);
        float safeDirectIntensity = max(directLightIntensity, 0.0);
        float safeAmbientIntensity = max(ambientIntensity, 0.0);
        vec3 directLighting = (diffuse + specular) * lightColor * (NdotL * shadow * safeDirectIntensity);

        vec3 ambientLight = ambientColor * safeAmbientIntensity;
        vec3 ambientDiffuse = ambientLight * baseColor * (1.0 - metallic) * (1.0 - transmission);
        vec3 ambientSpec = ambientLight * F0 * (1.0 - roughness) * 0.15;
        vec3 ambientLighting = ambientDiffuse + ambientSpec;
        vec3 shadedColor = directLighting + ambientLighting + emission;

        if (transmission > 0.001) {
            vec3 I = -V;
            float eta = 1.0 / max(ior, 1.0001);
            vec3 refracted = refract(I, N, eta);
            if (dot(refracted, refracted) < 1e-6) {
                refracted = reflect(I, N);
            }
            vec3 transmitted = clamp(backgroundColor, vec3(0.0), vec3(1.0));
            if (uBackgroundGradient == 1) {
                transmitted = clamp(transmitted + vec3(0.1), vec3(0.0), vec3(1.0));
            }
            float dielectricF0 = pow((ior - 1.0) / (ior + 1.0), 2.0);
            float fresnel = fresnelSchlick(max(dot(N, V), 0.0), vec3(dielectricF0)).r;
            float transmitWeight = transmission * (1.0 - fresnel);
            vec3 tint = mix(vec3(1.0), baseColor, 0.2);
            vec3 glassColor = transmitted * tint + emission;
            color = mix(shadedColor, glassColor, clamp(transmitWeight, 0.0, 1.0));
        } else {
            color = shadedColor;
        }
    } else {
        vec3 baseColor = clamp(backgroundColor, vec3(0.0), vec3(1.0));
        if (uBackgroundGradient == 1) {
            baseColor = clamp(baseColor + vec3(0.1), vec3(0.0), vec3(1.0));
        }
        color = baseColor;
    }

    if (uRenderMode == 0) {
        if (uSelectedCount > 0) {
            float combinedEdge = 0.0;
            for (int i = 0; i < uSelectedCount; ++i) {
                vec4 selEdge = rayMarchSelectedFor(i, ro, rd);
                combinedEdge = max(combinedEdge, selEdge.z);
            }
            if (combinedEdge > 0.0) {
                vec3 outlineColor = vec3(1.0, 0.5, 0.0);
                color = mix(color, outlineColor, combinedEdge);
            }
        }

        if (uShowAxisGuides == 1 && uActiveAxis >= 0) {
            vec4 axisGuide = rayMarchAxisGuide(ro, rd, uGuideCenter, uGuideAxisDirection);
            if (axisGuide.z > 0.0) {
                vec3 axisColor = vec3(1.0, 0.0, 0.0);
                if (uActiveAxis == 0) axisColor = vec3(1.0, 0.0, 0.0);
                else if (uActiveAxis == 1) axisColor = vec3(0.0, 0.0, 1.0);
                else if (uActiveAxis == 2) axisColor = vec3(0.0, 1.0, 0.0);

                color = mix(color, axisColor, 0.4);
            }
        }

        if (uMirrorHelperShow == 1) {
            float nearestHelperT = MAX_DIST;
            vec3 nearestHelperColor = vec3(0.0);
            float nearestHelperAlpha = 0.0;
            float helperExtent = max(uMirrorHelperExtent, 0.1);
            float helperLineWidth = max(0.03 * helperExtent, 0.03);
            float helperGridSpacing = max(helperExtent * 0.25, 0.25);

            for (int axis = 0; axis < 3; ++axis) {
                float enabled = (axis == 0) ? uMirrorHelperAxisEnabled.x :
                                ((axis == 1) ? uMirrorHelperAxisEnabled.y : uMirrorHelperAxisEnabled.z);
                if (enabled < 0.5) {
                    continue;
                }

                float roA = (axis == 0) ? ro.x : ((axis == 1) ? ro.y : ro.z);
                float rdA = (axis == 0) ? rd.x : ((axis == 1) ? rd.y : rd.z);
                float planeOffset = (axis == 0) ? uMirrorHelperOffset.x :
                                    ((axis == 1) ? uMirrorHelperOffset.y : uMirrorHelperOffset.z);
                if (abs(rdA) < 1e-6) {
                    continue;
                }

                float tPlane = (planeOffset - roA) / rdA;
                if (tPlane <= 0.0 || tPlane >= dist || tPlane >= nearestHelperT) {
                    continue;
                }

                vec3 pPlane = ro + rd * tPlane;
                vec2 planeUV;
                vec2 planeCenter;
                vec3 planeColor;
                if (axis == 0) {
                    planeUV = pPlane.yz;
                    planeCenter = uMirrorHelperCenter.yz;
                    planeColor = vec3(1.0, 0.2, 0.2);
                } else if (axis == 1) {
                    planeUV = pPlane.xz;
                    planeCenter = uMirrorHelperCenter.xz;
                    planeColor = vec3(0.2, 0.2, 1.0);
                } else {
                    planeUV = pPlane.xy;
                    planeCenter = uMirrorHelperCenter.xy;
                    planeColor = vec3(0.2, 1.0, 0.2);
                }
                bool isSelectedPlane = (uMirrorHelperSelected == 1) && (axis == uMirrorHelperSelectedAxis);
                if (isSelectedPlane) {
                    planeColor = mix(planeColor, vec3(1.0), 0.5);
                }

                vec2 local = planeUV - planeCenter;
                if (abs(local.x) > helperExtent || abs(local.y) > helperExtent) {
                    continue;
                }

                vec2 gridUV = local / helperGridSpacing;
                vec2 gridLines = abs(fract(gridUV - 0.5) - 0.5) / fwidth(gridUV);
                float gridPattern = min(gridLines.x, gridLines.y);
                float gridIntensity = 1.0 - smoothstep(0.0, 1.0, gridPattern);

                float borderDist = abs(max(abs(local.x), abs(local.y)) - helperExtent);
                float borderIntensity = 1.0 - smoothstep(0.0, helperLineWidth, borderDist);

                float helperAlpha = max(gridIntensity * 0.45, borderIntensity * 0.95);
                if (isSelectedPlane) {
                    helperAlpha = max(helperAlpha, 0.82);
                }
                if (helperAlpha > 0.001) {
                    nearestHelperT = tPlane;
                    nearestHelperColor = planeColor;
                    nearestHelperAlpha = helperAlpha;
                }
            }

            if (nearestHelperAlpha > 0.0) {
                color = mix(color, nearestHelperColor, clamp(nearestHelperAlpha, 0.0, 0.9));
            }
        }

        if (abs(rd.y) > 0.001) {
            float tPlane = -ro.y / rd.y;
            if (tPlane > 0.0 && tPlane < MAX_DIST) {
                float camOriginDist = length(cameraPos);
                vec3 pPlane = ro + rd * tPlane;
                float gridScale = mix(2.0, 0.5, clamp(camOriginDist, 0.0, 0.5));
                vec2 gridUV = pPlane.xz * gridScale;
                vec2 gridLines = abs(fract(gridUV - 0.5) - 0.5) / fwidth(gridUV);
                float gridPattern = min(gridLines.x, gridLines.y);
                float gridIntensity = 1.0 - smoothstep(0.0, 1.0, gridPattern);
                vec3 baseGridColor = vec3(0.35, 0.35, 0.35);
                float axisThreshold = 0.01;
                if (abs(pPlane.x) < axisThreshold)
                    baseGridColor = mix(baseGridColor, vec3(0.0, 1.0, 0.0), 0.3);
                if (abs(pPlane.z) < axisThreshold)
                    baseGridColor = mix(baseGridColor, vec3(1.0, 0.0, 0.0), 0.3);
                float fogDistance = 25.0;
                float fogFactor = clamp(1.0 - tPlane / fogDistance, 0.0, 1.0);
                if (tPlane < dist)
                    color = mix(color, baseGridColor, gridIntensity * fogFactor);
            }
        }
    }

    color = AgXToneMapping(color);
    fragColor = vec4(color, 1.0);
}

void main() {
    mainImage(FragColor, fragCoord);
}
