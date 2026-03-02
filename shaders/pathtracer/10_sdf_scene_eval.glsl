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
    res.mat.dispersion = mix(scene.mat.dispersion, candidate.mat.dispersion, h);
    res.mat.albedoTexIndex = mix(scene.mat.albedoTexIndex, candidate.mat.albedoTexIndex, h);
    res.mat.roughnessTexIndex = mix(scene.mat.roughnessTexIndex, candidate.mat.roughnessTexIndex, h);
    res.mat.metallicTexIndex = mix(scene.mat.metallicTexIndex, candidate.mat.metallicTexIndex, h);
    res.mat.normalTexIndex = mix(scene.mat.normalTexIndex, candidate.mat.normalTexIndex, h);
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
    res.mat.dispersion = mix(scene.mat.dispersion, candidate.mat.dispersion, h);
    res.mat.albedoTexIndex = mix(scene.mat.albedoTexIndex, candidate.mat.albedoTexIndex, h);
    res.mat.roughnessTexIndex = mix(scene.mat.roughnessTexIndex, candidate.mat.roughnessTexIndex, h);
    res.mat.metallicTexIndex = mix(scene.mat.metallicTexIndex, candidate.mat.metallicTexIndex, h);
    res.mat.normalTexIndex = mix(scene.mat.normalTexIndex, candidate.mat.normalTexIndex, h);
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
    res.mat.dispersion = mix(scene.mat.dispersion, candidate.mat.dispersion, h);
    res.mat.albedoTexIndex = mix(scene.mat.albedoTexIndex, candidate.mat.albedoTexIndex, h);
    res.mat.roughnessTexIndex = mix(scene.mat.roughnessTexIndex, candidate.mat.roughnessTexIndex, h);
    res.mat.metallicTexIndex = mix(scene.mat.metallicTexIndex, candidate.mat.metallicTexIndex, h);
    res.mat.normalTexIndex = mix(scene.mat.normalTexIndex, candidate.mat.normalTexIndex, h);
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

void getArrayCandidateIndices(float pAxis,
                              float spacing,
                              float repeatCount,
                              float smoothness,
                              out ivec3 indices,
                              out int count) {
    int repeats = max(int(floor(repeatCount + 0.5)), 1);
    float center = 0.5 * float(repeats - 1);
    float safeSpacing = max(abs(spacing), 1e-4);
    float coord = pAxis / safeSpacing + center;
    int nearest = clamp(int(floor(coord + 0.5)), 0, repeats - 1);

    indices = ivec3(nearest);
    count = 1;
    if(smoothness <= 1e-6 || repeats <= 1) {
        return;
    }

    int a = clamp(nearest - 1, 0, repeats - 1);
    int b = nearest;
    int c = clamp(nearest + 1, 0, repeats - 1);

    if(a == b && b == c) {
        indices = ivec3(nearest);
        count = 1;
    } else if(a == b) {
        indices = ivec3(a, c, c);
        count = 2;
    } else if(b == c) {
        indices = ivec3(a, b, b);
        count = 2;
    } else {
        indices = ivec3(a, b, c);
        count = 3;
    }
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

float sdCapsuleVariableRadiusLocal(vec3 p, vec3 a, vec3 b, float ra, float rb) {
    vec3 ab = b - a;
    float lenSq = max(dot(ab, ab), 1e-8);
    float t = clamp(dot(p - a, ab) / lenSq, 0.0, 1.0);
    vec3 c = a + ab * t;
    float r = mix(ra, rb, t);
    return length(p - c) - max(r, 1e-4);
}

float catmullRomScalar(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5 * (
        (2.0 * p1) +
        (-p0 + p2) * t +
        (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
        (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3
    );
}

vec3 catmullRomVec3(vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t) {
    return vec3(
        catmullRomScalar(p0.x, p1.x, p2.x, p3.x, t),
        catmullRomScalar(p0.y, p1.y, p2.y, p3.y, t),
        catmullRomScalar(p0.z, p1.z, p2.z, p3.z, t)
    );
}

float sdCurveLocal(vec3 p, vec4 param) {
    int nodeOffset = int(floor(param.x + 0.5));
    int nodeCount = int(floor(param.y + 0.5));
    if(nodeCount <= 0 || nodeOffset < 0 || nodeOffset >= uCurveNodeCount) {
        return MAX_DIST;
    }

    nodeCount = min(nodeCount, uCurveNodeCount - nodeOffset);
    if(nodeCount <= 0) {
        return MAX_DIST;
    }

    vec4 n0 = texelFetch(uCurveNodeBuffer, nodeOffset);
    if(nodeCount == 1) {
        return length(p - n0.xyz) - max(n0.w, 1e-4);
    }

    const int CURVE_SUBSEGMENTS = 12;
    float d = MAX_DIST;
    for(int i = 0; i < 63; ++i) {
        if(i >= nodeCount - 1) {
            break;
        }

        int i0 = max(i - 1, 0);
        int i1 = i;
        int i2 = i + 1;
        int i3 = min(i + 2, nodeCount - 1);

        vec4 p0 = texelFetch(uCurveNodeBuffer, nodeOffset + i0);
        vec4 p1 = texelFetch(uCurveNodeBuffer, nodeOffset + i1);
        vec4 p2 = texelFetch(uCurveNodeBuffer, nodeOffset + i2);
        vec4 p3 = texelFetch(uCurveNodeBuffer, nodeOffset + i3);

        vec3 prevPos = catmullRomVec3(p0.xyz, p1.xyz, p2.xyz, p3.xyz, 0.0);
        float prevRadius = max(catmullRomScalar(p0.w, p1.w, p2.w, p3.w, 0.0), 1e-4);

        for(int sub = 1; sub <= CURVE_SUBSEGMENTS; ++sub) {
            float t = float(sub) / float(CURVE_SUBSEGMENTS);
            vec3 currPos = catmullRomVec3(p0.xyz, p1.xyz, p2.xyz, p3.xyz, t);
            float currRadius = max(catmullRomScalar(p0.w, p1.w, p2.w, p3.w, t), 1e-4);
            float segmentDist = sdCapsuleVariableRadiusLocal(p, prevPos, currPos, prevRadius, currRadius);
            d = min(d, segmentDist);
            prevPos = currPos;
            prevRadius = currRadius;
        }
    }
    return d;
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
    } else if(type == 8) {
        return sdCurveLocal(p, param);
    }
    return MAX_DIST;
}

float applyDeformScale(int type, vec3 localP, vec4 param, vec2 fractalExtra, vec3 scale) {
    vec3 s = safeScale(scale);
    float k = min(min(s.x, s.y), s.z);
    return sdPrimitiveLocal(type, localP / s, param, fractalExtra) * k;
}

float evaluateShapeDistanceFromLocal(ShapeData shape, vec3 localP) {
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

float evaluateShapeDistanceSingle(ShapeData shape, vec3 p) {
    vec3 localP = p - shape.center;
    if(hasFlag(shape.flags, FLAG_HAS_ROTATION)) {
        localP = vec3(dot(shape.invRotRow0, localP), dot(shape.invRotRow1, localP), dot(shape.invRotRow2, localP));
    }

    vec3 candidates[27];
    int candidateCount = 1;
    candidates[0] = localP;

    bool usedArrayModifier = false;
    float arraySmoothK = max(shape.arraySmoothness, 0.0);

    for(int stackIndex = 0; stackIndex < 3; ++stackIndex) {
        int modifierId = clamp(int(floor(shape.modifierStack[stackIndex] + 0.5)), SHAPE_MODIFIER_BEND, SHAPE_MODIFIER_ARRAY);

        if(modifierId == SHAPE_MODIFIER_TWIST && hasFlag(shape.flags, FLAG_HAS_TWIST)) {
            for(int i = 0; i < candidateCount; ++i) {
                candidates[i] = applyTwistCombined(candidates[i], shape.twist);
            }
        } else if(modifierId == SHAPE_MODIFIER_BEND && hasFlag(shape.flags, FLAG_HAS_BEND)) {
            for(int i = 0; i < candidateCount; ++i) {
                candidates[i] = applyBendCombined(candidates[i], shape.bend);
            }
        } else if(modifierId == SHAPE_MODIFIER_ARRAY && hasFlag(shape.flags, FLAG_HAS_ARRAY)) {
            vec3 expanded[27];
            int expandedCount = 0;
            int repeatX = max(int(floor(shape.arrayRepeatCount.x + 0.5)), 1);
            int repeatY = max(int(floor(shape.arrayRepeatCount.y + 0.5)), 1);
            int repeatZ = max(int(floor(shape.arrayRepeatCount.z + 0.5)), 1);
            float centerX = 0.5 * float(repeatX - 1);
            float centerY = 0.5 * float(repeatY - 1);
            float centerZ = 0.5 * float(repeatZ - 1);

            for(int baseIndex = 0; baseIndex < candidateCount; ++baseIndex) {
                ivec3 xIndices = ivec3(0);
                ivec3 yIndices = ivec3(0);
                ivec3 zIndices = ivec3(0);
                int xCount = 1;
                int yCount = 1;
                int zCount = 1;

                if(shape.arrayAxis.x > 0.5) {
                    getArrayCandidateIndices(candidates[baseIndex].x, shape.arraySpacing.x, shape.arrayRepeatCount.x, arraySmoothK, xIndices, xCount);
                }
                if(shape.arrayAxis.y > 0.5) {
                    getArrayCandidateIndices(candidates[baseIndex].y, shape.arraySpacing.y, shape.arrayRepeatCount.y, arraySmoothK, yIndices, yCount);
                }
                if(shape.arrayAxis.z > 0.5) {
                    getArrayCandidateIndices(candidates[baseIndex].z, shape.arraySpacing.z, shape.arrayRepeatCount.z, arraySmoothK, zIndices, zCount);
                }

                for(int xi = 0; xi < xCount; ++xi) {
                    for(int yi = 0; yi < yCount; ++yi) {
                        for(int zi = 0; zi < zCount; ++zi) {
                            if(expandedCount >= 27) {
                                continue;
                            }

                            int xIndex = (xi == 0) ? xIndices.x : ((xi == 1) ? xIndices.y : xIndices.z);
                            int yIndex = (yi == 0) ? yIndices.x : ((yi == 1) ? yIndices.y : yIndices.z);
                            int zIndex = (zi == 0) ? zIndices.x : ((zi == 1) ? zIndices.y : zIndices.z);

                            vec3 sampleP = candidates[baseIndex];
                            if(shape.arrayAxis.x > 0.5) {
                                sampleP.x -= (float(xIndex) - centerX) * shape.arraySpacing.x;
                            }
                            if(shape.arrayAxis.y > 0.5) {
                                sampleP.y -= (float(yIndex) - centerY) * shape.arraySpacing.y;
                            }
                            if(shape.arrayAxis.z > 0.5) {
                                sampleP.z -= (float(zIndex) - centerZ) * shape.arraySpacing.z;
                            }
                            expanded[expandedCount++] = sampleP;
                        }
                    }
                }
            }

            if(expandedCount > 0) {
                candidateCount = expandedCount;
                for(int i = 0; i < candidateCount; ++i) {
                    candidates[i] = expanded[i];
                }
                usedArrayModifier = true;
            }
        }
    }

    float d = MAX_DIST;
    for(int i = 0; i < candidateCount; ++i) {
        float candidate = evaluateShapeDistanceFromLocal(shape, candidates[i]);
        if(i == 0) {
            d = candidate;
        } else if(usedArrayModifier && arraySmoothK > 1e-6) {
            d = opSmoothUnionDistance(candidate, d, arraySmoothK);
        } else {
            d = min(d, candidate);
        }
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

float evaluateShapeDistanceWithDisplacement(ShapeData shape, vec3 p) {
    float d = evaluateShapeDistance(shape, p);
    if(shape.displacementStrength <= 1e-6 || !hasMaterialTexture(shape.displacementTexIndex)) {
        return d;
    }

    vec3 displacementNormal = p - shape.center;
    if(dot(displacementNormal, displacementNormal) < 1e-8) {
        displacementNormal = vec3(0.0, 1.0, 0.0);
    }
    float displacement = sampleDisplacementTexture(shape.displacementTexIndex,
                                                   shape.displacementStrength,
                                                   p,
                                                   displacementNormal);
    return d - displacement;
}

SDFResult sdfForShape(ShapeData shape, vec3 p, vec3 shadingNormal) {
    SDFResult res;
    res.dist = evaluateShapeDistanceWithDisplacement(shape, p);
    res.mat.albedo = sampleAlbedoTexture(shape.albedo, shape.albedoTexIndex, p, shadingNormal);
    res.mat.metallic = sampleMetallicTexture(shape.metallic, shape.metallicTexIndex, p, shadingNormal);
    res.mat.roughness = sampleRoughnessTexture(shape.roughness, shape.roughnessTexIndex, p, shadingNormal);
    res.mat.emission = shape.emission;
    res.mat.emissionStrength = shape.emissionStrength;
    res.mat.transmission = shape.transmission;
    res.mat.ior = shape.ior;
    res.mat.dispersion = shape.dispersion;
    res.mat.albedoTexIndex = shape.albedoTexIndex;
    res.mat.roughnessTexIndex = shape.roughnessTexIndex;
    res.mat.metallicTexIndex = shape.metallicTexIndex;
    res.mat.normalTexIndex = shape.normalTexIndex;
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
    float candidateDist = evaluateShapeDistanceWithDisplacement(shape, p);

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

SDFResult accumulateSceneFullForShape(vec3 p, vec3 shadingNormal, int shapeIndex, SDFResult res) {
    vec4 t0;
    vec4 t1;
    int blendOp;
    vec3 center;
    float influenceRadius;
    float smoothness;
    loadShapeHeader(shapeIndex, t0, t1, blendOp, center, influenceRadius, smoothness);

    vec3 centerDelta = p - center;
    float centerDistSq = dot(centerDelta, centerDelta);
    if(canSkipShape(blendOp, centerDistSq, influenceRadius, res.dist, smoothness)) {
        return res;
    }

    ShapeData shape = loadShapeDataFromHeader(shapeIndex, t0, t1);
    SDFResult candidate = sdfForShape(shape, p, shadingNormal);

    if(blendOp == BLEND_SMOOTH_UNION) {
        return opSmoothUnion(candidate, res, smoothness);
    }
    if(blendOp == BLEND_SMOOTH_SUBTRACTION) {
        return opSmoothSubtraction(candidate, res, smoothness);
    }
    if(blendOp == BLEND_SMOOTH_INTERSECTION) {
        return opSmoothIntersection(candidate, res, smoothness);
    }
    if(candidate.dist < res.dist) {
        return candidate;
    }
    return res;
}

SDFResult sceneSDFFullForAccelRange(vec3 p, vec3 shadingNormal, int accelStart, int accelCount) {
    SDFResult res;
    res.dist = MAX_DIST;
    res.mat.albedo = vec3(0.0);
    res.mat.metallic = 0.0;
    res.mat.roughness = 1.0;
    res.mat.emission = vec3(0.0);
    res.mat.emissionStrength = 0.0;
    res.mat.transmission = 0.0;
    res.mat.ior = 1.5;
    res.mat.dispersion = 0.0;
    res.mat.albedoTexIndex = -1.0;
    res.mat.roughnessTexIndex = -1.0;
    res.mat.metallicTexIndex = -1.0;
    res.mat.normalTexIndex = -1.0;
    for(int j = 0; j < accelCount; ++j) {
        res = accumulateSceneFullForShape(p, shadingNormal, fetchAccelShapeIndex(accelStart + j), res);
    }
    return res;
}

float sceneSDFDistanceLowerBound(vec3 p) {
    float dist = MAX_DIST;
    for(int i = 0; i < shapeCount; ++i) {
        dist = accumulateSceneDistanceLowerBoundForShape(p, i, dist);
    }

    return dist;
}

float sceneSDFDistanceExact(vec3 p) {
    float dist = MAX_DIST;
    for(int i = 0; i < shapeCount; ++i) {
        dist = accumulateSceneDistanceExactForShape(p, i, dist);
    }

    return dist;
}

SDFResult sceneSDFFull(vec3 p, vec3 shadingNormal) {
