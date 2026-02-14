    float h = clamp(0.5 - 0.5 * (sceneDist - candidateDist) / k, 0.0, 1.0);
    return mix(sceneDist, candidateDist, h) + k * h * (1.0 - h);
}

vec3 safeScale(vec3 s) {
    return max(abs(s), vec3(0.001));
}

vec3 opTwistPoint(vec3 p, float k, int axis) {
    if(abs(k) < 1e-6)
        return p;
    int ax = clamp(axis, 0, 2);
    if(ax == 0) {
        float c = cos(k * p.x);
        float s = sin(k * p.x);
        mat2 m = mat2(c, -s, s, c);
        vec2 yz = m * p.yz;
        return vec3(p.x, yz.x, yz.y);
    } else if(ax == 1) {
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
    if(abs(k) < 1e-6)
        return p;
    int ax = clamp(axis, 0, 2);
    if(ax == 0) {
        float c = cos(k * p.y);
        float s = sin(k * p.y);
        mat2 m = mat2(c, -s, s, c);
        vec2 yz = m * p.yz;
        return vec3(p.x, yz.x, yz.y);
    } else if(ax == 1) {
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

float sdMandelbulbLocal(
    vec3 p,
    float power,
    float iterationsF,
    float bailout,
    float derivativeBias,
    float distanceScale
) {
    float safePower = clamp(power, 2.0, 16.0);
    int iterations = int(clamp(floor(iterationsF + 0.5), 1.0, 64.0));
    float bailoutRadius = max(bailout, 2.0);
    float safeDerivativeBias = clamp(derivativeBias, 0.01, 4.0);
    float safeDistanceScale = max(distanceScale, 0.01);

    vec3 z = p;
    float dr = 1.0;
    float r = 0.0;

    for(int i = 0; i < 64; ++i) {
        if(i >= iterations) {
            break;
        }

        r = length(z);
        if(r > bailoutRadius) {
            break;
        }

        float safeR = max(r, 1e-6);
        float theta = acos(clamp(z.z / safeR, -1.0, 1.0));
        float phi = atan(z.y, z.x);
        float zr = pow(safeR, safePower);
        dr = pow(safeR, safePower - 1.0) * safePower * dr + safeDerivativeBias;

        float t = theta * safePower;
        float pAngle = phi * safePower;
        float sinT = sin(t);
        z = zr * vec3(sinT * cos(pAngle), sinT * sin(pAngle), cos(t)) + p;
    }

    r = max(r, 1e-6);
    return safeDistanceScale * (0.5 * log(r) * r / max(dr, 1e-6));
}

float sdMengerSpongeLocal(vec3 p, float halfExtent, float iterationsF, float holeScale) {
    float safeHalfExtent = max(halfExtent, 0.01);
    int iterations = int(clamp(floor(iterationsF + 0.5), 1.0, 8.0));
    float carveScale = clamp(holeScale, 2.0, 4.0);

    vec3 q = p / safeHalfExtent;
    float d = sdBoxLocal(q, vec3(1.0));
    float scale = 1.0;

    for(int i = 0; i < 8; ++i) {
        if(i >= iterations) {
            break;
        }

        vec3 a = mod(q * scale, 2.0) - 1.0;
        scale *= 3.0;
        vec3 r = abs(1.0 - carveScale * abs(a));
        float carve = (min(max(r.x, r.y), min(max(r.y, r.z), max(r.z, r.x))) - 1.0) / scale;
        d = max(d, carve);
    }

    return d * safeHalfExtent;
}

float sampleMeshSdfGrid(int meshId, ivec3 cell, int res) {
    int linear = cell.x + res * (cell.y + res * cell.z);
    int volumeBase = meshId * res * res * res;
    return texelFetch(uMeshSdfBuffer, volumeBase + linear).x;
}

float sdMeshSdfLocal(vec3 p, vec4 param) {
    if(uMeshSdfResolution <= 1) {
        return MAX_DIST;
    }

    int meshId = int(param.y + 0.5);
    if(meshId < 0 || meshId >= uMeshSdfCount) {
        return MAX_DIST;
    }

    float baseScale = max(param.x, 0.001);
    vec3 normalized = p / baseScale;
    vec3 clamped = clamp(normalized, vec3(-1.0), vec3(1.0));
    vec3 outside = max(abs(normalized) - vec3(1.0), vec3(0.0));
    float outsideLen = length(outside);

    int res = uMeshSdfResolution;
    vec3 grid = clamp((clamped * 0.5 + 0.5) * float(res), vec3(0.0), vec3(float(res - 1)));
    ivec3 i0 = clamp(ivec3(floor(grid)), ivec3(0), ivec3(res - 1));
    ivec3 i1 = min(i0 + ivec3(1), ivec3(res - 1));
    vec3 t = clamp(grid - vec3(i0), vec3(0.0), vec3(1.0));

    float c000 = sampleMeshSdfGrid(meshId, ivec3(i0.x, i0.y, i0.z), res);
    float c100 = sampleMeshSdfGrid(meshId, ivec3(i1.x, i0.y, i0.z), res);
    float c010 = sampleMeshSdfGrid(meshId, ivec3(i0.x, i1.y, i0.z), res);
    float c110 = sampleMeshSdfGrid(meshId, ivec3(i1.x, i1.y, i0.z), res);
    float c001 = sampleMeshSdfGrid(meshId, ivec3(i0.x, i0.y, i1.z), res);
    float c101 = sampleMeshSdfGrid(meshId, ivec3(i1.x, i0.y, i1.z), res);
    float c011 = sampleMeshSdfGrid(meshId, ivec3(i0.x, i1.y, i1.z), res);
    float c111 = sampleMeshSdfGrid(meshId, ivec3(i1.x, i1.y, i1.z), res);

    float c00 = mix(c000, c100, t.x);
    float c10 = mix(c010, c110, t.x);
    float c01 = mix(c001, c101, t.x);
    float c11 = mix(c011, c111, t.x);
    float c0 = mix(c00, c10, t.y);
    float c1 = mix(c01, c11, t.y);
    float sampled = mix(c0, c1, t.z);
    float conservativeMargin = 0.5 * sqrt(3.0) * (2.0 / float(res));
    return (sampled + outsideLen - conservativeMargin) * baseScale;
}

float sdPrimitiveLocal(int type, vec3 p, vec4 param, vec2 fractalExtra) {
    if(type == 0) {
        return sdSphereLocal(p, param.w);
    } else if(type == 1) {
        return sdBoxLocal(p, param.xyz);
    } else if(type == 2) {
        return sdTorusLocal(p, vec2(param.x, param.y));
    } else if(type == 3) {
        return sdCylinderLocal(p, param.x, param.y);
    } else if(type == 4) {
        return sdConeLocal(p, param.x, param.y);
    } else if(type == 5) {
        return sdMandelbulbLocal(p, param.x, param.y, param.z, fractalExtra.x, fractalExtra.y);
    } else if(type == 6) {
        return sdMengerSpongeLocal(p, param.x, param.y, param.z);
    } else if(type == 7) {
        return sdMeshSdfLocal(p, param);
    }
    return MAX_DIST;
}

float applyDeformScale(int type, vec3 localP, vec4 param, vec2 fractalExtra, vec3 scale) {
    vec3 s = safeScale(scale);
    float k = min(min(s.x, s.y), s.z);
    return sdPrimitiveLocal(type, localP / s, param, fractalExtra) * k;
}

float evaluateShapeDistanceSingle(ShapeData shape, vec3 p) {
    vec3 localP = p - shape.center;

    if(hasFlag(shape.flags, FLAG_HAS_ROTATION)) {
        localP = vec3(dot(shape.invRotRow0, localP), dot(shape.invRotRow1, localP), dot(shape.invRotRow2, localP));
    }

    if(hasFlag(shape.flags, FLAG_HAS_TWIST)) {
        localP = applyTwistCombined(localP, shape.twist);
    }

    if(hasFlag(shape.flags, FLAG_HAS_BEND)) {
        localP = applyBendCombined(localP, shape.bend);
    }

    vec3 q = localP;
    float correction = 0.0;
    if(hasFlag(shape.flags, FLAG_HAS_ELONGATION)) {
        vec3 raw = abs(localP) - shape.elongation;
        q = max(raw, 0.0);
        correction = min(max(raw.x, max(raw.y, raw.z)), 0.0);
    }

    float d = 0.0;
    if(hasFlag(shape.flags, FLAG_HAS_SCALE)) {
        d = applyDeformScale(shape.type, q, shape.param, shape.fractalExtra, shape.scale) + correction;
    } else {
        d = sdPrimitiveLocal(shape.type, q, shape.param, shape.fractalExtra) + correction;
    }

    if(hasFlag(shape.flags, FLAG_HAS_ROUNDING)) {
        d -= max(shape.roundness, 0.0);
    }

    if(hasFlag(shape.flags, FLAG_HAS_TWIST) || hasFlag(shape.flags, FLAG_HAS_BEND)) {
        float warpAmount = dot(abs(shape.twist), vec3(1.0)) + dot(abs(shape.bend), vec3(1.0));
        float safety = 1.0 + 0.35 * min(warpAmount, 8.0);
        d /= safety;
    }

    return d;
}

float evaluateShapeDistance(ShapeData shape, vec3 p) {
    float d = evaluateShapeDistanceSingle(shape, p);
    if(!hasFlag(shape.flags, FLAG_HAS_MIRROR)) {
        return d;
    }

    int maxX = (shape.mirrorAxis.x > 0.5) ? 1 : 0;
    int maxY = (shape.mirrorAxis.y > 0.5) ? 1 : 0;
    int maxZ = (shape.mirrorAxis.z > 0.5) ? 1 : 0;
    if(maxX == 0 && maxY == 0 && maxZ == 0) {
        return d;
    }
    float mirrorK = max(shape.mirrorSmoothness, 0.0);

    for(int ix = 0; ix <= maxX; ++ix) {
        for(int iy = 0; iy <= maxY; ++iy) {
            for(int iz = 0; iz <= maxZ; ++iz) {
                if(ix == 0 && iy == 0 && iz == 0) {
                    continue;
                }

                vec3 reflectedP = p;
                if(ix == 1)
                    reflectedP.x = 2.0 * shape.mirrorOffset.x - reflectedP.x;
                if(iy == 1)
                    reflectedP.y = 2.0 * shape.mirrorOffset.y - reflectedP.y;
                if(iz == 1)
                    reflectedP.z = 2.0 * shape.mirrorOffset.z - reflectedP.z;
                float reflectedD = evaluateShapeDistanceSingle(shape, reflectedP);
                if(mirrorK > 1e-6) {
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
    if(threshold <= 0.0) {
        return true;
    }
    return centerDistSq >= threshold * threshold;
}

bool distanceLE(float centerDistSq, float threshold) {
    if(threshold < 0.0) {
        return false;
    }
    return centerDistSq <= threshold * threshold;
}

bool canSkipShape(int blendOp, float centerDistSq, float influenceRadius, float currentDist, float smoothness) {
    float k = max(smoothness, 1e-4);
    if(blendOp == BLEND_NONE) {
        return distanceGE(centerDistSq, currentDist + influenceRadius);
    }
    if(blendOp == BLEND_SMOOTH_UNION) {
        return distanceGE(centerDistSq, currentDist + k + influenceRadius);
    }
    if(blendOp == BLEND_SMOOTH_SUBTRACTION) {
        return distanceGE(centerDistSq, k - currentDist + influenceRadius);
    }
    if(blendOp == BLEND_SMOOTH_INTERSECTION) {
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

    if(blendOp == BLEND_SMOOTH_SUBTRACTION || blendOp == BLEND_SMOOTH_INTERSECTION) {
        return dist;
    }

    vec3 centerDelta = p - center;
    float centerDistSq = dot(centerDelta, centerDelta);
    if(canSkipShape(blendOp, centerDistSq, influenceRadius, dist, smoothness)) {
        return dist;
    }

    float lb = sqrt(centerDistSq) - influenceRadius;
    if(blendOp == BLEND_SMOOTH_UNION) {
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
    if(canSkipShape(blendOp, centerDistSq, influenceRadius, dist, smoothness)) {
        return dist;
    }

    ShapeData shape = loadShapeDistanceDataFromHeader(shapeIndex, t0, t1);
    float candidateDist = evaluateShapeDistance(shape, p);

    if(blendOp == BLEND_SMOOTH_UNION) {
        return opSmoothUnionDistance(candidateDist, dist, smoothness);
    }
    if(blendOp == BLEND_SMOOTH_SUBTRACTION) {
        return opSmoothSubtractionDistance(candidateDist, dist, smoothness);
    }
    if(blendOp == BLEND_SMOOTH_INTERSECTION) {
        return opSmoothIntersectionDistance(candidateDist, dist, smoothness);
    }
    return min(dist, candidateDist);
}

