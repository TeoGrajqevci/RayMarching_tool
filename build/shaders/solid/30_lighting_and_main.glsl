        lastD = dEval;
        dist += dEval;
        if(dist > maxDist) {
            break;
        }
    }

    return vec4(dist, 0.0, edge, 0.0);
}

vec3 AgXToneMapping(vec3 color) {
    vec3 aces = clamp((color * (color + 0.0245786) - 0.000090537) /
        (color * (0.983729 * color + 0.4329510) + 0.238081), 0.0, 1.0);
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
    if(dirLen < 1e-6) {
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

    for(int i = 0; i < maxIter; ++i) {
        vec3 pos = ro + rd * dist;
        float d = sdAxisLine(pos, center, axisDir) - lineWidth;
        if(d < minDist) {
            return vec4(dist, 0.0, 1.0, 0.0);
        }
        dist += d;
        if(dist > maxDist) {
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
    if(length(right) < 1e-6) {
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

void mainImage(out vec4 fragColor, in vec2 fragCoordIn) {
    vec2 uv = fragCoordIn * 0.5 + 0.5;
    vec3 ro;
    vec3 rd;
    computeCameraRay(uv, ro, rd);

    vec3 hitPos = ro;
    float dist = MAX_DIST;
    bool hitScene = false;
    int mainMarchSteps = 0;
    int shadowSamples = 0;
    int transmissionSamples = 0;
    int terminationCode = 3;
    float hitEpsAtHit = SURFACE_DIST;

    float boundsEnter;
    float boundsExit;
    if(intersectSceneBounds(ro, rd, boundsEnter, boundsExit)) {
        float marchDist = boundsEnter;
        float marchEnd = min(MAX_DIST, boundsExit);
        bool havePrev = false;
        float prevDist = MAX_DIST;
        float prevT = marchDist;
        int marchLimit = getMarchStepLimit();
        for(int i = 0; i < MAX_STEPS; ++i) {
            if(i >= marchLimit) {
                break;
            }
            mainMarchSteps += 1;
            hitPos = ro + rd * marchDist;
            float dScene = MAX_DIST;
            float cellExit = 0.0;

            ivec3 cell;
            int accelStart;
            int accelCount;
            bool forceFullScan;
            bool hasCell = getAccelCellData(hitPos, cell, accelStart, accelCount, forceFullScan);
            if(hasCell && !forceFullScan) {
                cellExit = accelCellExitDistance(hitPos, rd, cell);
                if(accelCount > 0) {
                    float dLower = sceneSDFDistanceLowerBoundForAccelRange(hitPos, accelStart, accelCount);
                    if(dLower <= ACCEL_EXACT_REFINE_DIST || cellExit <= ACCEL_MIN_CELL_EXIT_REFINE) {
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

            float hitEps = getAdaptiveHitEpsilon(marchDist);
            if(abs(dScene) < hitEps) {
                hitScene = true;
                dist = marchDist;
                hitEpsAtHit = hitEps;
                terminationCode = 1;
                break;
            }
            if(havePrev && prevDist > 0.0 && dScene < 0.0) {
                hitPos = refineSurfaceCrossing(ro, rd, prevT, marchDist);
                hitScene = true;
                dist = length(hitPos - ro);
                hitEpsAtHit = hitEps;
                terminationCode = 2;
                break;
            }

            float dAbs = abs(dScene);
            float overRelax = getSafeOverRelax(dAbs, hitEps);
            float stepDist = max(dAbs * SPHERE_TRACE_SAFETY * overRelax, hitEps * 0.33);
            if(hasCell && !forceFullScan) {
                stepDist = min(stepDist, cellExit + hitEps * 0.5);
            }
            havePrev = true;
            prevDist = dScene;
            prevT = marchDist;
            marchDist += stepDist;
            if(marchDist > marchEnd) {
                break;
            }
        }
    } else {
        terminationCode = 4;
    }

    vec3 color = vec3(0.0);
    if(hitScene) {
        vec3 N = normalize(getNormal(hitPos, getAdaptiveNormalEpsilon(dist)));
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

        float shadow = softShadow(hitPos + N * hitEpsAtHit * 2.0, L, 0.001, MAX_DIST, 32.0, shadowSamples);
        float safeDirectIntensity = max(directLightIntensity, 0.0);
        float safeAmbientIntensity = max(ambientIntensity, 0.0);
        vec3 directLighting = (diffuse + specular) * lightColor * (NdotL * shadow * safeDirectIntensity);

        vec3 pointLighting = vec3(0.0);
        int pointCount = clamp(uPointLightCount, 0, MAX_POINT_LIGHTS);
        for(int pointLightIndex = 0; pointLightIndex < pointCount; ++pointLightIndex) {
            vec3 pointPos = uPointLightPos[pointLightIndex];
            float pointRadius = max(uPointLightRadius[pointLightIndex], 0.0);

            vec3 toPointCenter = pointPos - hitPos;
            float toPointCenterLen = length(toPointCenter);
            if(toPointCenterLen <= 1e-4) {
                continue;
            }

            vec3 pointL = toPointCenter / toPointCenterLen;
            float pointDist = max(toPointCenterLen - pointRadius, 1e-4);
            float pointNdotL = max(dot(N, pointL), 0.0);
            if(pointNdotL <= 0.0) {
                continue;
            }

            vec3 pointH = normalize(pointL + V);
            float pointNdotH = max(dot(N, pointH), 0.0);
            float pointHdotV = max(dot(pointH, V), 0.0);

            vec3 pointF = fresnelSchlick(pointHdotV, F0);
            float pointD = distributionGGX(pointNdotH, roughness);
            float pointG = geometrySmith(NdotV, pointNdotL, roughness);

            vec3 pointSpecular = (pointD * pointG * pointF) / max(4.0 * NdotV * pointNdotL, 1e-4);
            vec3 pointKS = pointF;
            vec3 pointKD = (vec3(1.0) - pointKS) * (1.0 - metallic) * (1.0 - transmission);
            vec3 pointDiffuse = pointKD * baseColor / PI;

            float pointShadow = softShadow(hitPos + N * hitEpsAtHit * 2.0, pointL, 0.001, pointDist, 32.0, shadowSamples);
            float pointRange = max(uPointLightRange[pointLightIndex], 0.001);
            float attenuation = 1.0 / (1.0 + (pointDist * pointDist) / (pointRange * pointRange));
            float areaFalloff = 1.0 / max(pointDist * pointDist, 1e-4);
            float softAreaBoost = 1.0 + pointRadius * 2.0;
            float pointScale = attenuation * areaFalloff * softAreaBoost;

            float pointIntensity = max(uPointLightIntensity[pointLightIndex], 0.0);
            vec3 pointDirect = (pointDiffuse + pointSpecular) *
                uPointLightColor[pointLightIndex] *
                (pointNdotL * pointShadow * pointIntensity * pointScale);
            pointLighting += pointDirect;
        }

        vec3 ambientLight = ambientColor * safeAmbientIntensity;
        vec3 ambientDiffuse = ambientLight * baseColor * (1.0 - metallic) * (1.0 - transmission);
        vec3 ambientSpec = ambientLight * F0 * (1.0 - roughness) * 0.15;
        vec3 ambientLighting = ambientDiffuse + ambientSpec;
        vec3 shadedColor = directLighting + pointLighting + ambientLighting + emission;

        if(transmission > 0.001) {
            vec3 I = -V;
            float eta = 1.0 / max(ior, 1.0001);
            vec3 refracted = refract(I, N, eta);
            if(dot(refracted, refracted) < 1e-6) {
                refracted = reflect(I, N);
            }
            vec3 transmitted = clamp(backgroundColor, vec3(0.0), vec3(1.0));
            if(uBackgroundGradient == 1) {
                transmitted = clamp(transmitted + vec3(0.1), vec3(0.0), vec3(1.0));
            }

            vec3 throughHitPos;
            SDFResult throughHitRes;
            vec3 refrDir = normalize(refracted);
            if(marchSceneTransmission(hitPos + refrDir * (hitEpsAtHit * 6.0), refrDir, throughHitPos, throughHitRes, transmissionSamples)) {
                vec3 throughBase = max(throughHitRes.mat.albedo, vec3(0.0));
                vec3 throughEmission = max(throughHitRes.mat.emission, vec3(0.0)) * max(throughHitRes.mat.emissionStrength, 0.0);
                transmitted = ambientLight * throughBase + throughEmission;
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
        if(uBackgroundGradient == 1) {
            baseColor = clamp(baseColor + vec3(0.1), vec3(0.0), vec3(1.0));
        }
        color = baseColor;
    }

    if(uDebugOutputMode == DEBUG_OUTPUT_COUNTERS) {
        int encodedTermination = clamp(terminationCode, 0, 255);
        vec3 encodedCounters = vec3(float(clamp(mainMarchSteps, 0, 255)), float(clamp(shadowSamples, 0, 255)), float(clamp(transmissionSamples, 0, 255))) / 255.0;
        fragColor = vec4(encodedCounters, float(encodedTermination) / 255.0);
        return;
    }

    if(uRenderMode == 0) {
        if(uSelectedCount > 0) {
            float combinedEdge = 0.0;
            for(int i = 0; i < uSelectedCount; ++i) {
                vec4 selEdge = rayMarchSelectedFor(i, ro, rd);
                combinedEdge = max(combinedEdge, selEdge.z);
            }
            if(combinedEdge > 0.0) {
                vec3 outlineColor = vec3(1.0, 0.5, 0.0);
                color = mix(color, outlineColor, combinedEdge);
            }
        }

        if(uShowAxisGuides == 1 && uActiveAxis >= 0) {
            vec4 axisGuide = rayMarchAxisGuide(ro, rd, uGuideCenter, uGuideAxisDirection);
            if(axisGuide.z > 0.0) {
                vec3 axisColor = vec3(1.0, 0.0, 0.0);
                if(uActiveAxis == 0)
                    axisColor = vec3(1.0, 0.0, 0.0);
                else if(uActiveAxis == 1)
                    axisColor = vec3(0.0, 0.0, 1.0);
                else if(uActiveAxis == 2)
                    axisColor = vec3(0.0, 1.0, 0.0);

                color = mix(color, axisColor, 0.4);
            }
        }

        if(uMirrorHelperShow == 1) {
            float nearestHelperT = MAX_DIST;
            vec3 nearestHelperColor = vec3(0.0);
            float nearestHelperAlpha = 0.0;
            float helperExtent = max(uMirrorHelperExtent, 0.1);
            float helperLineWidth = max(0.03 * helperExtent, 0.03);
            float helperGridSpacing = max(helperExtent * 0.25, 0.25);

            for(int axis = 0; axis < 3; ++axis) {
                float enabled = (axis == 0) ? uMirrorHelperAxisEnabled.x : ((axis == 1) ? uMirrorHelperAxisEnabled.y : uMirrorHelperAxisEnabled.z);
                if(enabled < 0.5) {
                    continue;
                }

                float roA = (axis == 0) ? ro.x : ((axis == 1) ? ro.y : ro.z);
                float rdA = (axis == 0) ? rd.x : ((axis == 1) ? rd.y : rd.z);
                float planeOffset = (axis == 0) ? uMirrorHelperOffset.x : ((axis == 1) ? uMirrorHelperOffset.y : uMirrorHelperOffset.z);
                if(abs(rdA) < 1e-6) {
                    continue;
                }

                float tPlane = (planeOffset - roA) / rdA;
                if(tPlane <= 0.0 || tPlane >= dist || tPlane >= nearestHelperT) {
                    continue;
                }

                vec3 pPlane = ro + rd * tPlane;
                vec2 planeUV;
                vec2 planeCenter;
                vec3 planeColor;
                if(axis == 0) {
                    planeUV = pPlane.yz;
                    planeCenter = uMirrorHelperCenter.yz;
                    planeColor = vec3(1.0, 0.2, 0.2);
                } else if(axis == 1) {
                    planeUV = pPlane.xz;
                    planeCenter = uMirrorHelperCenter.xz;
                    planeColor = vec3(0.2, 0.2, 1.0);
                } else {
                    planeUV = pPlane.xy;
                    planeCenter = uMirrorHelperCenter.xy;
                    planeColor = vec3(0.2, 1.0, 0.2);
                }
                bool isSelectedPlane = (uMirrorHelperSelected == 1) && (axis == uMirrorHelperSelectedAxis);
                if(isSelectedPlane) {
                    planeColor = mix(planeColor, vec3(1.0), 0.5);
                }

                vec2 local = planeUV - planeCenter;
                if(abs(local.x) > helperExtent || abs(local.y) > helperExtent) {
                    continue;
                }

                vec2 gridUV = local / helperGridSpacing;
                vec2 gridLines = abs(fract(gridUV - 0.5) - 0.5) / fwidth(gridUV);
                float gridPattern = min(gridLines.x, gridLines.y);
                float gridIntensity = 1.0 - smoothstep(0.0, 1.0, gridPattern);

                float borderDist = abs(max(abs(local.x), abs(local.y)) - helperExtent);
                float borderIntensity = 1.0 - smoothstep(0.0, helperLineWidth, borderDist);

                float helperAlpha = max(gridIntensity * 0.45, borderIntensity * 0.95);
                if(isSelectedPlane) {
                    helperAlpha = max(helperAlpha, 0.82);
                }
                if(helperAlpha > 0.001) {
                    nearestHelperT = tPlane;
                    nearestHelperColor = planeColor;
                    nearestHelperAlpha = helperAlpha;
                }
            }

            if(nearestHelperAlpha > 0.0) {
                color = mix(color, nearestHelperColor, clamp(nearestHelperAlpha, 0.0, 0.9));
            }
        }

        if(abs(rd.y) > 0.001) {
            float tPlane = -ro.y / rd.y;
            if(tPlane > 0.0 && tPlane < MAX_DIST) {
                float camOriginDist = length(cameraPos);
                vec3 pPlane = ro + rd * tPlane;
                float gridScale = mix(2.0, 0.5, clamp(camOriginDist, 0.0, 0.5));
                vec2 gridUV = pPlane.xz * gridScale;
                vec2 gridLines = abs(fract(gridUV - 0.5) - 0.5) / fwidth(gridUV);
                float gridPattern = min(gridLines.x, gridLines.y);
                float gridIntensity = 1.0 - smoothstep(0.0, 1.0, gridPattern);
                vec3 baseGridColor = vec3(0.35, 0.35, 0.35);
                float axisThreshold = 0.01;
                if(abs(pPlane.x) < axisThreshold)
                    baseGridColor = mix(baseGridColor, vec3(0.0, 1.0, 0.0), 0.3);
                if(abs(pPlane.z) < axisThreshold)
                    baseGridColor = mix(baseGridColor, vec3(1.0, 0.0, 0.0), 0.3);
                float fogDistance = 25.0;
                float fogFactor = clamp(1.0 - tPlane / fogDistance, 0.0, 1.0);
                if(tPlane < dist)
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
