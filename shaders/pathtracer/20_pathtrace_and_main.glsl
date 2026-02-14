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

SDFResult sceneSDFFullAt(vec3 p) {
    if(uOptimizedMarchEnabled == 0) {
        return sceneSDFFull(p);
    }
    int accelStart;
    int accelCount;
    bool forceFullScan;
    bool hasCell = getAccelCellRange(p, accelStart, accelCount, forceFullScan);
    if(hasCell && !forceFullScan && accelCount > 0) {
        return sceneSDFFullForAccelRange(p, accelStart, accelCount);
    }
    return sceneSDFFull(p);
}

vec3 getNormal(vec3 p, float eps) {
    const vec3 k1 = vec3(1.0, -1.0, -1.0);
    const vec3 k2 = vec3(-1.0, -1.0, 1.0);
    const vec3 k3 = vec3(-1.0, 1.0, -1.0);
    const vec3 k4 = vec3(1.0, 1.0, 1.0);

    vec3 n = k1 * sceneSDFDistanceExactAt(p + k1 * eps) +
        k2 * sceneSDFDistanceExactAt(p + k2 * eps) +
        k3 * sceneSDFDistanceExactAt(p + k3 * eps) +
        k4 * sceneSDFDistanceExactAt(p + k4 * eps);
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
        float dm = sceneSDFDistanceExactAt(ro + rd * m);
        if(dm > 0.0) {
            a = m;
        } else {
            b = m;
        }
    }
    return ro + rd * (0.5 * (a + b));
}

bool marchScene(vec3 ro, vec3 rd, out HitInfo hit) {
    float boundsEnter;
    float boundsExit;
    if(!intersectSceneBounds(ro, rd, boundsEnter, boundsExit)) {
        return false;
    }

    float t = max(boundsEnter, 0.0);
    float marchEnd = min(MAX_DIST, boundsExit);
    bool havePrev = false;
    float prevT = t;
    float prevDist = MAX_DIST;
    int marchLimit = getMarchStepLimit();
    for(int i = 0; i < MAX_MARCH_STEPS; ++i) {
        if(i >= marchLimit) {
            break;
        }
        vec3 pos = ro + rd * t;
        float dist = MAX_DIST;
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
                if(dApprox <= ACCEL_EXACT_REFINE_DIST || cellExit <= ACCEL_MIN_CELL_EXIT_REFINE) {
                    dist = sceneSDFDistanceExact(pos);
                } else {
                    dist = dApprox;
                }
            } else {
                dist = cellExit;
            }
        } else {
            dist = sceneSDFDistanceExact(pos);
        }

        float hitEps = getAdaptiveHitEpsilon(t);
        if(abs(dist) < hitEps) {
            hit.position = pos;
            hit.normal = getNormal(pos, getAdaptiveNormalEpsilon(t));
            SDFResult full = sceneSDFFullAt(pos);
            hit.mat = full.mat;
            return true;
        }
        if(havePrev && prevDist > 0.0 && dist < 0.0) {
            hit.position = refineSurfaceCrossing(ro, rd, prevT, t);
            hit.normal = getNormal(hit.position, getAdaptiveNormalEpsilon(t));
            SDFResult full = sceneSDFFullAt(hit.position);
            hit.mat = full.mat;
            return true;
        }

        float dAbs = abs(dist);
        float overRelax = getSafeOverRelax(dAbs, hitEps);
        float stepDist = max(dAbs * SPHERE_TRACE_SAFETY * overRelax, hitEps * 0.33);
        if(hasCell && !forceFullScan) {
            stepDist = min(stepDist, cellExit + hitEps * 0.5);
        }
        havePrev = true;
        prevT = t;
        prevDist = dist;
        t += stepDist;
        if(t >= marchEnd) {
            break;
        }
    }
    return false;
}

float traceShadowVisibility(vec3 ro, vec3 rd, float maxShadowDist) {
    if(uOptimizedMarchEnabled == 0) {
        float visibility = 1.0;
        vec3 shadowOrigin = ro;

        for(int layer = 0; layer < 8; ++layer) {
            HitInfo occHit;
            if(!marchScene(shadowOrigin, rd, occHit)) {
                break;
            }

            if(length(occHit.position - ro) >= maxShadowDist) {
                break;
            }

            float transmission = clamp(occHit.mat.transmission, 0.0, 1.0);
            if(transmission < 0.02) {
                return 0.0;
            }

            float tint = clamp(dot(max(occHit.mat.albedo, vec3(0.0)), vec3(0.3333333)), 0.1, 1.0);
            float mediumTrans = clamp(mix(0.2, 1.0, transmission), 0.05, 1.0);
            visibility *= mediumTrans * mix(1.0, tint, transmission * 0.35);

            if(visibility < 0.01) {
                return 0.0;
            }

            shadowOrigin = occHit.position + rd * (SURFACE_DIST * 8.0);
        }

        return clamp(visibility, 0.0, 1.0);
    }

    float visibility = 1.0;
    vec3 shadowOrigin = ro;
    int maxLayers = getShadowLayerLimit();

    for(int layer = 0; layer < 8; ++layer) {
        if(layer >= maxLayers) {
            break;
        }

        float boundsEnter;
        float boundsExit;
        if(!intersectSceneBounds(shadowOrigin, rd, boundsEnter, boundsExit)) {
            break;
        }

        float t = max(boundsEnter, 0.0);
        float marchEnd = min(maxShadowDist, boundsExit);
        bool havePrev = false;
        float prevT = t;
        float prevDist = MAX_DIST;
        bool foundHit = false;
        vec3 occPos = shadowOrigin;

        int shadowStepLimit = max(24, getMarchStepLimit() / 2);
        for(int i = 0; i < MAX_MARCH_STEPS; ++i) {
            if(i >= shadowStepLimit) {
                break;
            }

            vec3 pos = shadowOrigin + rd * t;
            float dist = MAX_DIST;
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
                    if(dApprox <= ACCEL_EXACT_REFINE_SHADOW_DIST || cellExit <= ACCEL_MIN_CELL_EXIT_REFINE) {
                        dist = sceneSDFDistanceExactAt(pos);
                    } else {
                        dist = dApprox;
                    }
                } else {
                    dist = cellExit;
                }
            } else {
                dist = sceneSDFDistanceExactAt(pos);
            }

            float hitEps = SURFACE_DIST;
            if(abs(dist) < hitEps) {
                occPos = pos;
                foundHit = true;
                break;
            }
            if(havePrev && prevDist > 0.0 && dist < 0.0) {
                occPos = refineSurfaceCrossing(shadowOrigin, rd, prevT, t);
                foundHit = true;
                break;
            }

            float dAbs = abs(dist);
            float stepDist = max(dAbs * SPHERE_TRACE_SAFETY, SURFACE_DIST * 0.25);
            if(hasCell && !forceFullScan) {
                stepDist = min(stepDist, cellExit + hitEps * 0.5);
            }
            havePrev = true;
            prevT = t;
            prevDist = dist;
            t += stepDist;
            if(t >= marchEnd) {
                break;
            }
        }

        if(!foundHit) {
            break;
        }

        SDFResult occ = sceneSDFFullAt(occPos);
        float transmission = clamp(occ.mat.transmission, 0.0, 1.0);
        if(transmission < 0.02) {
            return 0.0;
        }

        float tint = clamp(dot(max(occ.mat.albedo, vec3(0.0)), vec3(0.3333333)), 0.1, 1.0);
        float mediumTrans = clamp(mix(0.2, 1.0, transmission), 0.05, 1.0);
        visibility *= mediumTrans * mix(1.0, tint, transmission * 0.35);
        if(visibility < 0.01) {
            return 0.0;
        }
        shadowOrigin = occPos + rd * (SURFACE_DIST * 8.0);
    }

    return clamp(visibility, 0.0, 1.0);
}

float traceShadowVisibility(vec3 ro, vec3 rd) {
    return traceShadowVisibility(ro, rd, MAX_DIST);
}

vec3 sampleSky(vec3 rd) {
    vec3 baseColor = clamp(backgroundColor, vec3(0.0), vec3(1.0));
    if(uBackgroundGradient == 1) {
        baseColor = clamp(baseColor + vec3(0.1), vec3(0.0), vec3(1.0));
    }
    return baseColor;
}

uint pcgHash(uint inputValue) {
    uint state = inputValue * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float rand01(inout uint state) {
    state = pcgHash(state);
    return float(state) / 4294967296.0;
}

uint initSeed(ivec2 pixel, int frame) {
    uint seed = uint(pixel.x) * 1973u + uint(pixel.y) * 9277u + uint(frame) * 26699u + 911u;
    if(uUseFixedSeed == 1) {
        seed ^= uint(max(uFixedSeed, 0));
    } else {
        seed ^= uint(iTime * 1000.0);
    }
    return pcgHash(seed);
}

void buildBasis(vec3 n, out vec3 tangent, out vec3 bitangent) {
    vec3 up = abs(n.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    tangent = normalize(cross(up, n));
    bitangent = cross(n, tangent);
}

vec3 sampleCosineHemisphere(vec3 n, inout uint state) {
    float u1 = rand01(state);
    float u2 = rand01(state);
    float r = sqrt(u1);
    float phi = 2.0 * PI * u2;

    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(max(0.0, 1.0 - u1));

    vec3 tangent;
    vec3 bitangent;
    buildBasis(n, tangent, bitangent);

    return normalize(tangent * x + bitangent * y + n * z);
}

vec3 sampleUniformSphere(inout uint state) {
    float u1 = rand01(state);
    float u2 = rand01(state);
    float z = 1.0 - 2.0 * u1;
    float r = sqrt(max(0.0, 1.0 - z * z));
    float phi = 2.0 * PI * u2;
    return vec3(r * cos(phi), r * sin(phi), z);
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

vec3 tracePath(vec3 ro, vec3 rd, inout uint rngState) {
    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);
    int bounceLimit = clamp(uMaxBounces, 1, MAX_BOUNCES);
    bool causticChainActive = false;
    vec3 causticIncomingDir = vec3(0.0, 0.0, 1.0);
    float causticStrength = 0.0;
    float causticSharpness = 32.0;

    for(int bounce = 0; bounce < MAX_BOUNCES; ++bounce) {
        if(bounce >= bounceLimit) {
            break;
        }

        HitInfo hit;
        if(!marchScene(ro, rd, hit)) {
            radiance += throughput * sampleSky(rd);
            break;
        }

        float metallic = clamp(hit.mat.metallic, 0.0, 1.0);
        float roughness = clamp(hit.mat.roughness, 0.04, 1.0);
        float transmission = clamp(hit.mat.transmission, 0.0, 1.0);
        float ior = max(hit.mat.ior, 1.0);
        vec3 albedo = max(hit.mat.albedo, vec3(0.0));
        vec3 emission = max(hit.mat.emission, vec3(0.0)) * max(hit.mat.emissionStrength, 0.0);
        radiance += throughput * emission;

        vec3 outwardNormal = normalize(hit.normal);
        bool frontFace = dot(rd, outwardNormal) < 0.0;
        vec3 N = frontFace ? outwardNormal : -outwardNormal;

        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        vec3 V = -rd;
        vec3 L = normalize(lightDir);
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float safeDirectIntensity = max(directLightIntensity, 0.0);
        float safeAmbientIntensity = max(ambientIntensity, 0.0);

        float ambientWeight = (bounce == 0) ? 1.0 : 0.0;
        vec3 ambientLight = ambientColor * safeAmbientIntensity * ambientWeight;
        vec3 ambientDiffuse = ambientLight * albedo * (1.0 - metallic) * (1.0 - transmission);
        vec3 ambientSpec = ambientLight * F0 * (1.0 - roughness) * 0.15 * (1.0 - transmission);
        radiance += throughput * (ambientDiffuse + ambientSpec);

        if(NdotL > 0.0) {
            float visibility = traceShadowVisibility(hit.position + N * (SURFACE_DIST * 6.0), L);
            vec3 H = normalize(L + V);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);

            vec3 F = fresnelSchlick(HdotV, F0);
            float D = distributionGGX(NdotH, roughness);
            float G = geometrySmith(NdotV, NdotL, roughness);

            vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-4);
            vec3 kS = F;
            vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic) * (1.0 - transmission);
            vec3 diffuse = kD * albedo / PI;

            vec3 direct = (diffuse + specular) * lightColor * (NdotL * visibility * safeDirectIntensity);
            radiance += throughput * direct;
        }

        for(int pointLightIndex = 0; pointLightIndex < uPointLightCount; ++pointLightIndex) {
            vec3 pointCenter = uPointLightPos[pointLightIndex];
            float pointRadius = max(uPointLightRadius[pointLightIndex], 0.0);
            vec3 sampledPoint = pointCenter;
            if(pointRadius > 1e-4) {
                sampledPoint += sampleUniformSphere(rngState) * pointRadius;
            }

            vec3 toPoint = sampledPoint - hit.position;
            float pointDist = length(toPoint);
            if(pointDist > 1e-4) {
                vec3 pointL = toPoint / pointDist;
                float pointNdotL = max(dot(N, pointL), 0.0);
                if(pointNdotL > 0.0) {
                    float pointVisibility = traceShadowVisibility(hit.position + N * (SURFACE_DIST * 6.0), pointL, pointDist);
                    vec3 pointH = normalize(pointL + V);
                    float pointNdotH = max(dot(N, pointH), 0.0);
                    float pointHdotV = max(dot(pointH, V), 0.0);

                    vec3 pointF = fresnelSchlick(pointHdotV, F0);
                    float pointD = distributionGGX(pointNdotH, roughness);
                    float pointG = geometrySmith(NdotV, pointNdotL, roughness);

                    vec3 pointSpecular = (pointD * pointG * pointF) / max(4.0 * NdotV * pointNdotL, 1e-4);
                    vec3 pointKS = pointF;
                    vec3 pointKD = (vec3(1.0) - pointKS) * (1.0 - metallic) * (1.0 - transmission);
                    vec3 pointDiffuse = pointKD * albedo / PI;

                    float pointRange = max(uPointLightRange[pointLightIndex], 0.001);
                    float attenuation = 1.0 / (1.0 + (pointDist * pointDist) / (pointRange * pointRange));
                    float areaFalloff = 1.0 / max(pointDist * pointDist, 1e-4);
                    float softAreaBoost = 1.0 + pointRadius * 2.0;
                    float lightScale = attenuation * areaFalloff * softAreaBoost;

                    float causticBoost = 1.0;
                    if(causticChainActive && transmission < 0.4) {
                        float focus = pow(max(dot(pointL, -causticIncomingDir), 0.0), causticSharpness);
                        causticBoost += causticStrength * focus;
                    }
                    vec3 pointDirect = (pointDiffuse + pointSpecular) * uPointLightColor[pointLightIndex] *
                        (pointNdotL * pointVisibility * max(uPointLightIntensity[pointLightIndex], 0.0) * lightScale * causticBoost);
                    radiance += throughput * pointDirect;
                }
            }
        }

        float etaI = frontFace ? 1.0 : max(ior, 1.0001);
        float etaT = frontFace ? max(ior, 1.0001) : 1.0;
        float eta = etaI / etaT;
        float cosTheta = clamp(dot(-rd, N), 0.0, 1.0);
        float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
        bool cannotRefract = eta * sinTheta > 1.0;

        float r0 = (etaI - etaT) / max(etaI + etaT, 1e-4);
        r0 *= r0;
        float dielectricFres = r0 + (1.0 - r0) * pow(1.0 - cosTheta, 5.0);

        vec3 Fv = fresnelSchlick(NdotV, F0);
        float specWeightBase = clamp((Fv.r + Fv.g + Fv.b) / 3.0, 0.02, 0.98);
        float diffuseWeight = max((1.0 - metallic) * (1.0 - transmission), 0.0);
        float reflectWeight = max(mix(specWeightBase, dielectricFres, transmission), 0.0);
        float refractWeight = transmission * max(1.0 - dielectricFres, 0.0);
        if(cannotRefract) {
            reflectWeight += refractWeight;
            refractWeight = 0.0;
        }

        float weightSum = diffuseWeight + reflectWeight + refractWeight;
        if(weightSum <= 1e-5) {
            break;
        }

        float diffuseProb = diffuseWeight / weightSum;
        float reflectProb = reflectWeight / weightSum;
        float refractProb = refractWeight / weightSum;
        float chooser = rand01(rngState);
        vec3 nextDir;

        if(chooser < reflectProb) {
            vec3 reflected = reflect(rd, N);
            vec3 reflectedGloss = sampleCosineHemisphere(reflected, rngState);
            nextDir = normalize(mix(reflected, reflectedGloss, roughness * roughness));
            vec3 reflectTint = mix(Fv, vec3(dielectricFres), transmission);
            throughput *= reflectTint / max(reflectProb, 0.001);

            causticChainActive = false;
            causticStrength = 0.0;
        } else if(chooser < reflectProb + refractProb) {
            vec3 refracted = refract(rd, N, eta);
            if(dot(refracted, refracted) < 1e-6) {
                refracted = reflect(rd, N);
            }
            vec3 refractedGloss = sampleCosineHemisphere(refracted, rngState);
            nextDir = normalize(mix(refracted, refractedGloss, roughness * roughness * 0.35));
            vec3 transTint = mix(vec3(1.0), albedo, 0.2);
            throughput *= transTint / max(refractProb, 0.001);

            causticChainActive = true;
            causticIncomingDir = nextDir;
            causticStrength = clamp(transmission * ior * (1.0 - roughness) * 3.0, 0.0, 8.0);
            causticSharpness = mix(16.0, 96.0, clamp(1.0 - roughness, 0.0, 1.0));
        } else {
            nextDir = sampleCosineHemisphere(N, rngState);
            vec3 diffuseTint = (vec3(1.0) - Fv) * (1.0 - metallic) * (1.0 - transmission) * albedo;
            throughput *= diffuseTint / max(diffuseProb, 0.001);

            causticChainActive = false;
            causticStrength = 0.0;
        }

        throughput = clamp(throughput, vec3(0.0), vec3(25.0));

        if(bounce >= 2) {
            float rr = clamp(max(throughput.r, max(throughput.g, throughput.b)), 0.05, 0.98);
            if(rand01(rngState) > rr) {
                break;
            }
            throughput /= rr;
        }

        ro = hit.position + nextDir * (SURFACE_DIST * 6.0);
        rd = nextDir;
    }

    return clamp(radiance, vec3(0.0), vec3(64.0));
}

void computeCameraRay(vec2 uv, out vec3 ro, out vec3 rd) {
    vec2 pScreen = (uv - 0.5) * 2.0;
    pScreen.x *= iResolution.x / max(iResolution.y, 1.0);

    vec3 forward = normalize(cameraTarget - cameraPos);
    vec3 right = normalize(cross(forward, vec3(0.0, 1.0, 0.0)));
    if(length(right) < 1e-5) {
        right = vec3(1.0, 0.0, 0.0);
    }
    vec3 up = normalize(cross(right, forward));

    if(uCameraProjectionMode == 1) {
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

void main() {
    vec2 uv = fragCoord * 0.5 + 0.5;
    vec3 ro;
    vec3 rd;
    computeCameraRay(uv, ro, rd);

    uint rngState = initSeed(ivec2(gl_FragCoord.xy), uFrameIndex);
    vec3 sampleColor = tracePath(ro, rd, rngState);

    vec3 prevAccum = texture(uPrevAccum, uv).rgb;
    float prevCount = float(max(uSampleCount, 0));
    vec3 accum = (prevAccum * prevCount + sampleColor) / (prevCount + 1.0);

    FragColor = vec4(accum, 1.0);
}
