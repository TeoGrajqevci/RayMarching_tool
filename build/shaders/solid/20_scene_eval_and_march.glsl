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
    if(canSkipShape(blendOp, centerDistSq, influenceRadius, res.dist, smoothness)) {
        return res;
    }

    ShapeData shape = loadShapeDataFromHeader(shapeIndex, t0, t1);
    SDFResult candidate = sdfForShape(shape, p);

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
    for(int i = 0; i < shapeCount; ++i) {
        res = accumulateSceneFullForShape(p, i, res);
    }

    return res;
}

vec3 getNormal(vec3 p, float eps) {
    const vec3 k1 = vec3(1.0, -1.0, -1.0);
    const vec3 k2 = vec3(-1.0, -1.0, 1.0);
    const vec3 k3 = vec3(-1.0, 1.0, -1.0);
    const vec3 k4 = vec3(1.0, 1.0, 1.0);

    vec3 n = k1 * sceneSDFDistanceExact(p + k1 * eps) +
        k2 * sceneSDFDistanceExact(p + k2 * eps) +
        k3 * sceneSDFDistanceExact(p + k3 * eps) +
        k4 * sceneSDFDistanceExact(p + k4 * eps);
    return normalize(n);
}

bool intersectSceneBounds(vec3 ro, vec3 rd, out float tEnter, out float tExit) {
    if(uSceneBoundsRadius <= 0.0) {
        tEnter = 0.0;
        tExit = -1.0;
        return false;
    }

    vec3 oc = ro - uSceneBoundsCenter;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - uSceneBoundsRadius * uSceneBoundsRadius;
    float h = b * b - c;
    if(h < 0.0) {
        tEnter = 0.0;
        tExit = -1.0;
        return false;
    }

    float s = sqrt(h);
    tEnter = -b - s;
    tExit = -b + s;
    if(tExit < 0.0) {
        return false;
    }

    tEnter = max(tEnter, 0.0);
    return true;
}

vec3 refineSurfaceCrossing(vec3 ro, vec3 rd, float tNear, float tFar) {
    float a = min(tNear, tFar);
    float b = max(tNear, tFar);
    for(int iter = 0; iter < 6; ++iter) {
        float m = 0.5 * (a + b);
        float dm = sceneSDFDistanceExact(ro + rd * m);
        if(dm > 0.0) {
            a = m;
        } else {
            b = m;
        }
    }
    return ro + rd * (0.5 * (a + b));
}

float softShadow(vec3 ro, vec3 rd, float mint, float maxt, float k, inout int shadowSteps) {
    float boundsEnter;
    float boundsExit;
    if(!intersectSceneBounds(ro, rd, boundsEnter, boundsExit)) {
        return 1.0;
    }

    float res = 1.0;
    float t = max(mint, boundsEnter);
    float maxT = min(maxt, boundsExit);
    if(t >= maxT) {
        return 1.0;
    }

    int shadowLimit = getShadowStepLimit();
    for(int i = 0; i < 24; ++i) {
        if(i >= shadowLimit) {
            break;
        }
        shadowSteps += 1;
        vec3 samplePos = ro + rd * t;
        float h = MAX_DIST;

        ivec3 cell;
        int accelStart;
        int accelCount;
        bool forceFullScan;
        bool hasCell = getAccelCellData(samplePos, cell, accelStart, accelCount, forceFullScan);
        float cellExit = 0.0;

        if(hasCell && !forceFullScan) {
            cellExit = accelCellExitDistance(samplePos, rd, cell);
            if(accelCount > 0) {
                float hLower = sceneSDFDistanceLowerBoundForAccelRange(samplePos, accelStart, accelCount);
                if(hLower <= ACCEL_EXACT_REFINE_SHADOW_DIST || cellExit <= ACCEL_MIN_CELL_EXIT_REFINE) {
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

        float hitEps = getAdaptiveHitEpsilon(t);
        if(h < 0.0 || abs(h) < hitEps * 0.5) {
            return 0.0;
        }
        float hAbs = abs(h);
        float overRelax = getSafeOverRelax(hAbs, hitEps);
        float stepDist = max(hAbs * SPHERE_TRACE_SAFETY * overRelax, hitEps * 0.33);
        if(hasCell && !forceFullScan) {
            stepDist = min(stepDist, cellExit + hitEps * 0.5);
        }
        res = min(res, k * stepDist / max(t, 0.001));
        if(res < 0.01) {
            return 0.0;
        }
        t += stepDist;
        if(t >= maxT) {
            break;
        }
    }
    return res;
}

bool marchSceneTransmission(vec3 ro, vec3 rd, out vec3 hitPos, out SDFResult hitRes, inout int transmissionSteps) {
    float boundsEnter;
    float boundsExit;
    if(!intersectSceneBounds(ro, rd, boundsEnter, boundsExit)) {
        return false;
    }

    float t = max(boundsEnter, 0.0);
    float marchEnd = min(MAX_DIST, boundsExit);
    bool havePrev = false;
    float prevT = t;
    float prevD = MAX_DIST;
    int marchLimit = min(MAX_STEPS, getMarchStepLimit() + 24);
    for(int i = 0; i < MAX_STEPS * 2; ++i) {
        if(i >= marchLimit) {
            break;
        }
        transmissionSteps += 1;
        vec3 pos = ro + rd * t;
        float dScene = MAX_DIST;
        float cellExit = 0.0;

        ivec3 cell;
        int accelStart;
        int accelCount;
        bool forceFullScan;
        bool hasCell = getAccelCellData(pos, cell, accelStart, accelCount, forceFullScan);
        if(hasCell && !forceFullScan) {
            cellExit = accelCellExitDistance(pos, rd, cell);
            if(accelCount > 0) {
                float dApprox = sceneSDFDistanceExactForAccelRange(pos, accelStart, accelCount);
                if(abs(dApprox) <= ACCEL_EXACT_REFINE_DIST || cellExit <= ACCEL_MIN_CELL_EXIT_REFINE) {
                    dScene = sceneSDFDistanceExact(pos);
                } else {
                    dScene = dApprox;
                }
            } else {
                dScene = cellExit;
            }
        } else {
            dScene = sceneSDFDistanceExact(pos);
        }

        float hitEps = getAdaptiveHitEpsilon(t);
        if(abs(dScene) < hitEps) {
            hitPos = pos;
            hitRes = sceneSDFFull(pos);
            return true;
        }

        if(havePrev && prevD > 0.0 && dScene < 0.0) {
            hitPos = refineSurfaceCrossing(ro, rd, prevT, t);
            hitRes = sceneSDFFull(hitPos);
            return true;
        }

        float dAbs = abs(dScene);
        float overRelax = getSafeOverRelax(dAbs, hitEps);
        float stepDist = max(dAbs * SPHERE_TRACE_SAFETY * overRelax, hitEps * 0.33);
        if(hasCell && !forceFullScan) {
            stepDist = min(stepDist, cellExit + hitEps * 0.5);
        }
        havePrev = true;
        prevT = t;
        prevD = dScene;
        t += stepDist;
        if(t >= marchEnd) {
            break;
        }
    }
    return false;
}

float selectedShapeSDFFor(int selectedIdx, vec3 p) {
    int shapeIndex = uSelectedIndices[selectedIdx];
    if(shapeIndex < 0 || shapeIndex >= shapeCount) {
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

    for(int i = 0; i < maxIter; ++i) {
        vec3 pos = ro + rd * dist;
        float dEval = selectedShapeSDFFor(idx, pos);
        if(dEval < minDist) {
            break;
        }
        if(lastD < uOutlineThickness && dEval > lastD + 0.001) {
            edge = 1.0;
            break;
        }
