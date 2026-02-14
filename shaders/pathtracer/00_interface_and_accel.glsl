
out vec4 FragColor;
in vec2 fragCoord;

uniform vec2 iResolution;
uniform float iTime;

uniform sampler2D uPrevAccum;
uniform int uSampleCount;
uniform int uFrameIndex;
uniform int uUseFixedSeed;
uniform int uFixedSeed;

uniform samplerBuffer uShapeBuffer;
uniform int uShapeTexelStride;
uniform int shapeCount;
uniform samplerBuffer uAccelCellRanges;
uniform samplerBuffer uAccelShapeIndices;
uniform int uAccelGridEnabled;
uniform ivec3 uAccelGridDim;
uniform vec3 uAccelGridMin;
uniform vec3 uAccelGridInvCellSize;
uniform samplerBuffer uMeshSdfBuffer;
uniform int uMeshSdfResolution;
uniform int uMeshSdfCount;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec3 backgroundColor;
uniform float ambientIntensity;
uniform float directLightIntensity;
const int MAX_POINT_LIGHTS = 16;
uniform int uPointLightCount;
uniform vec3 uPointLightPos[MAX_POINT_LIGHTS];
uniform vec3 uPointLightColor[MAX_POINT_LIGHTS];
uniform float uPointLightIntensity[MAX_POINT_LIGHTS];
uniform float uPointLightRange[MAX_POINT_LIGHTS];
uniform float uPointLightRadius[MAX_POINT_LIGHTS];

uniform vec3 cameraPos;
uniform vec3 cameraTarget;
uniform float uCameraFovDegrees;
uniform int uCameraProjectionMode;
uniform float uCameraOrthoScale;
uniform int uMaxBounces;
uniform int uRayMarchQuality;
uniform int uOptimizedMarchEnabled;
uniform int uBackgroundGradient;
uniform vec3 uSceneBoundsCenter;
uniform float uSceneBoundsRadius;

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

const float PI = 3.14159265359;
const int MAX_BOUNCES = 12;
const int MAX_MARCH_STEPS = 120;
const float MAX_DIST = 100.0;
const float SURFACE_DIST = 0.001;
const float SPHERE_TRACE_SAFETY = 0.7;
const float ACCEL_EXACT_REFINE_DIST = 0.004;
const float ACCEL_EXACT_REFINE_SHADOW_DIST = 0.004;
const float ACCEL_MIN_CELL_EXIT_REFINE = 0.002;

const int RAYMARCH_QUALITY_ULTRA = 0;
const int RAYMARCH_QUALITY_HIGH = 1;
const int RAYMARCH_QUALITY_MEDIUM = 2;
const int RAYMARCH_QUALITY_LOW = 3;

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

struct HitInfo {
    vec3 position;
    vec3 normal;
    Material mat;
};

struct ShapeData {
    int type;
    int blendOp;
    int flags;
    vec3 center;
    float influenceRadius;
    vec4 param;
    vec2 fractalExtra;
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

    if(uAccelGridEnabled == 0) {
        return false;
    }

    vec3 cellCoordF = floor((p - uAccelGridMin) * uAccelGridInvCellSize);
    if(cellCoordF.x < 0.0 || cellCoordF.y < 0.0 || cellCoordF.z < 0.0) {
        return false;
    }
    if(cellCoordF.x >= float(uAccelGridDim.x) ||
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

    if(uAccelGridEnabled == 0) {
        return false;
    }

    vec3 cellCoordF = floor((p - uAccelGridMin) * uAccelGridInvCellSize);
    if(cellCoordF.x < 0.0 || cellCoordF.y < 0.0 || cellCoordF.z < 0.0) {
        return false;
    }
    if(cellCoordF.x >= float(uAccelGridDim.x) ||
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

    if(rd.x > 1e-6) {
        tx = (cellMax.x - p.x) / rd.x;
    } else if(rd.x < -1e-6) {
        tx = (cellMin.x - p.x) / rd.x;
    }

    if(rd.y > 1e-6) {
        ty = (cellMax.y - p.y) / rd.y;
    } else if(rd.y < -1e-6) {
        ty = (cellMin.y - p.y) / rd.y;
    }

    if(rd.z > 1e-6) {
        tz = (cellMax.z - p.z) / rd.z;
    } else if(rd.z < -1e-6) {
        tz = (cellMin.z - p.z) / rd.z;
    }

    return max(min(tx, min(ty, tz)), 0.0);
}

float accumulateSceneDistanceExactForShape(vec3 p, int shapeIndex, float dist);

float sceneSDFDistanceExactForAccelRange(vec3 p, int accelStart, int accelCount) {
    float dist = MAX_DIST;
    for(int j = 0; j < accelCount; ++j) {
        dist = accumulateSceneDistanceExactForShape(p, fetchAccelShapeIndex(accelStart + j), dist);
    }
    return dist;
}

float sceneSDFDistanceExact(vec3 p);
SDFResult sceneSDFFull(vec3 p);

int getQualityLevel() {
    return clamp(uRayMarchQuality, RAYMARCH_QUALITY_ULTRA, RAYMARCH_QUALITY_LOW);
}

int getMarchStepLimit() {
    if(uOptimizedMarchEnabled == 0) {
        return MAX_MARCH_STEPS;
    }
    int quality = getQualityLevel();
    if(quality == RAYMARCH_QUALITY_ULTRA) {
        return 120;
    }
    if(quality == RAYMARCH_QUALITY_HIGH) {
        return 96;
    }
    if(quality == RAYMARCH_QUALITY_MEDIUM) {
        return 80;
    }
    return 64;
}

int getShadowLayerLimit() {
    if(uOptimizedMarchEnabled == 0) {
        return 8;
    }
    int quality = getQualityLevel();
    if(quality == RAYMARCH_QUALITY_ULTRA) {
        return 8;
    }
    if(quality == RAYMARCH_QUALITY_HIGH) {
        return 8;
    }
    if(quality == RAYMARCH_QUALITY_MEDIUM) {
        return 5;
    }
    return 4;
}

float getAdaptiveHitEpsilon(float t) {
    if(uOptimizedMarchEnabled == 0) {
        return SURFACE_DIST;
    }
    float tanHalfFov = tan(radians(clamp(uCameraFovDegrees, 1.0, 179.0)) * 0.5);
    float pixelWorld = (2.0 * tanHalfFov * max(t, 1.0)) / max(iResolution.y, 1.0);
    int quality = getQualityLevel();
    float qualityScale = 1.0;
    if(quality == RAYMARCH_QUALITY_ULTRA) {
        qualityScale = 0.70;
    } else if(quality == RAYMARCH_QUALITY_HIGH) {
        qualityScale = 0.90;
    } else if(quality == RAYMARCH_QUALITY_MEDIUM) {
        qualityScale = 1.30;
    } else {
        qualityScale = 1.80;
    }
    return clamp(max(SURFACE_DIST, pixelWorld * 0.65 * qualityScale), SURFACE_DIST * 0.75, SURFACE_DIST * 4.5);
}

float getAdaptiveNormalEpsilon(float t) {
    return clamp(getAdaptiveHitEpsilon(t) * 0.85, SURFACE_DIST * 0.5, SURFACE_DIST * 6.0);
}

float getSafeOverRelax(float dAbs, float hitEps) {
    if(uOptimizedMarchEnabled == 0) {
        return 1.0;
    }
    float nearSurface = clamp(dAbs / max(hitEps * 4.0, 1e-6), 0.0, 1.0);
    return mix(1.0, 1.24, nearSurface);
}

float sceneSDFDistanceExactAt(vec3 p) {
    if(uOptimizedMarchEnabled == 0) {
        return sceneSDFDistanceExact(p);
    }
    int accelStart;
    int accelCount;
    bool forceFullScan;
    bool hasCell = getAccelCellRange(p, accelStart, accelCount, forceFullScan);
    if(hasCell && !forceFullScan && accelCount > 0) {
        return sceneSDFDistanceExactForAccelRange(p, accelStart, accelCount);
    }
    return sceneSDFDistanceExact(p);
}

bool hasFlag(int flags, int bit) {
    return (flags & bit) != 0;
}

void loadShapeHeader(
    int shapeIndex,
    out vec4 t0,
    out vec4 t1,
    out int blendOp,
    out vec3 center,
    out float influenceRadius,
    out float smoothness
) {
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
    shape.fractalExtra = vec2(0.0);

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

    if(hasFlag(shape.flags, FLAG_HAS_ROTATION)) {
        vec4 t3 = fetchShapeTexel(shapeIndex, 3);
        vec4 t4 = fetchShapeTexel(shapeIndex, 4);
        vec4 t5 = fetchShapeTexel(shapeIndex, 5);
        shape.invRotRow0 = t3.xyz;
        shape.invRotRow1 = t4.xyz;
        shape.invRotRow2 = t5.xyz;
        if(hasFlag(shape.flags, FLAG_HAS_ROUNDING)) {
            shape.roundness = max(t4.w, 0.0);
        }
    } else if(hasFlag(shape.flags, FLAG_HAS_ROUNDING)) {
        vec4 t4 = fetchShapeTexel(shapeIndex, 4);
        shape.roundness = max(t4.w, 0.0);
    }

    if(hasFlag(shape.flags, FLAG_HAS_SCALE)) {
        vec4 t6 = fetchShapeTexel(shapeIndex, 6);
        shape.scale = t6.xyz;
    }
    if(hasFlag(shape.flags, FLAG_HAS_ELONGATION)) {
        vec4 t7 = fetchShapeTexel(shapeIndex, 7);
        shape.elongation = t7.xyz;
    }
    if(hasFlag(shape.flags, FLAG_HAS_TWIST)) {
        vec4 t8 = fetchShapeTexel(shapeIndex, 8);
        shape.twist = t8.xyz;
    }
    if(hasFlag(shape.flags, FLAG_HAS_BEND)) {
        vec4 t9 = fetchShapeTexel(shapeIndex, 9);
        shape.bend = t9.xyz;
    }
    if(hasFlag(shape.flags, FLAG_HAS_MIRROR)) {
        vec4 t6 = fetchShapeTexel(shapeIndex, 6);
        vec4 t7 = fetchShapeTexel(shapeIndex, 7);
        vec4 t8 = fetchShapeTexel(shapeIndex, 8);
        vec4 t9 = fetchShapeTexel(shapeIndex, 9);
        vec4 t11 = fetchShapeTexel(shapeIndex, 11);
        shape.mirrorAxis = clamp(t11.yzw, vec3(0.0), vec3(1.0));
        shape.mirrorOffset = vec3(t6.w, t7.w, t8.w);
        shape.mirrorSmoothness = max(t9.w, 0.0);
    }

    vec4 t13 = fetchShapeTexel(shapeIndex, 13);
    shape.fractalExtra = t13.zw;

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

