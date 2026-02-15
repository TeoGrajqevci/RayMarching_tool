SDFResult res;
res.dist = MAX_DIST;
res.mat.albedo = vec3(0.0);
res.mat.metallic = 0.0;
res.mat.roughness = 1.0;
res.mat.emission = vec3(0.0);
res.mat.emissionStrength = 0.0;
res.mat.transmission = 0.0;
res.mat.ior = 1.5;
for(int i = 0;
i < shapeCount;
++ i) {
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
if(hasCell && ! forceFullScan && accelCount > 0) {
return sceneSDFFullForAccelRange(p, accelStart, accelCount);
}
return sceneSDFFull(p);
}

vec3 getNormal(vec3 p, float eps) {
const vec3 k1 = vec3(1.0, - 1.0, - 1.0);
const vec3 k2 = vec3(- 1.0, - 1.0, 1.0);
const vec3 k3 = vec3(- 1.0, 1.0, - 1.0);
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
tExit = - 1.0;
return false;
}

vec3 oc = ro - uSceneBoundsCenter;
float b = dot(oc, rd);
float c = dot(oc, oc) - uSceneBoundsRadius * uSceneBoundsRadius;
float h = b * b - c;
if(h < 0.0) {
tEnter = 0.0;
tExit = - 1.0;
return false;
}

float s = sqrt(h);
tEnter = - b - s;
tExit = - b + s;
if(tExit < 0.0) {
return false;
}

tEnter = max(tEnter, 0.0);
return true;
}

vec3 refineSurfaceCrossing(vec3 ro, vec3 rd, float tNear, float tFar) {
float a = min(tNear, tFar);
float b = max(tNear, tFar);
for(int iter = 0;
iter < 6;
++ iter) {
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
if(! intersectSceneBounds(ro, rd, boundsEnter, boundsExit)) {
return false;
}

float t = max(boundsEnter, 0.0);
float marchEnd = min(MAX_DIST, boundsExit);
bool havePrev = false;
float prevT = t;
float prevDist = MAX_DIST;
int marchLimit = getMarchStepLimit();
for(int i = 0;
i < MAX_MARCH_STEPS;
++ i) {
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
if(hasCell && ! forceFullScan) {
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
if(hasCell && ! forceFullScan) {
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

vec3 sampleSky(vec3 rd) {
vec3 baseColor = clamp(backgroundColor, vec3(0.0), vec3(1.0));
if(uBackgroundGradient == 1) {
baseColor = clamp(baseColor + vec3(0.1), vec3(0.0), vec3(1.0));
}

vec3 L = normalize(lightDir);
float sunDot = max(dot(normalize(rd), L), 0.0);
float sunDisk = pow(sunDot, 2048.0);
float sunGlow = pow(sunDot, 32.0) * 0.05;
vec3 sunRadiance = lightColor * max(directLightIntensity, 0.0) * (sunDisk + sunGlow);

return baseColor + sunRadiance;
}

bool intersectPointLightEmitters(vec3 ro, vec3 rd, float maxDist, out float nearestT, out vec3 emittedRadiance) {
nearestT = maxDist;
emittedRadiance = vec3(0.0);
bool hitLight = false;

for(int pointLightIndex = 0;
pointLightIndex < uPointLightCount;
++ pointLightIndex) {
vec3 center = uPointLightPos[pointLightIndex];
float radius = max(uPointLightRadius[pointLightIndex], 0.05);

vec3 oc = ro - center;
float b = dot(oc, rd);
float c = dot(oc, oc) - radius * radius;
float h = b * b - c;
if(h < 0.0) {
continue;
}

float s = sqrt(h);
float tNear = - b - s;
float tFar = - b + s;
float tHit = (tNear > SURFACE_DIST * 2.0) ? tNear : tFar;
if(tHit <= SURFACE_DIST * 2.0 || tHit >= nearestT) {
continue;
}

nearestT = tHit;
hitLight = true;
float intensity = max(uPointLightIntensity[pointLightIndex], 0.0);
emittedRadiance = uPointLightColor[pointLightIndex] * intensity;
}

return hitLight;
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

vec3 sampleUniformCone(vec3 axis, float cosThetaMax, inout uint state) {
float u1 = rand01(state);
float u2 = rand01(state);
float cosTheta = mix(cosThetaMax, 1.0, u1);
float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
float phi = 2.0 * PI * u2;

vec3 tangent;
vec3 bitangent;
buildBasis(normalize(axis), tangent, bitangent);
return normalize(tangent * (cos(phi) * sinTheta) + bitangent * (sin(phi) * sinTheta) + normalize(axis) * cosTheta);
}

bool sampleLightGuidedDirection(vec3 pos, inout uint rngState, out vec3 guidedDir) {
int activePointLights = min(max(uPointLightCount, 0), MAX_POINT_LIGHTS);
bool hasSun = max(directLightIntensity, 0.0) > 1e-5;
int candidateCount = activePointLights + (hasSun ? 1 : 0);
if(candidateCount <= 0) {
guidedDir = vec3(0.0, 1.0, 0.0);
return false;
}

int chosen = int(floor(rand01(rngState) * float(candidateCount)));
chosen = clamp(chosen, 0, candidateCount - 1);

if(chosen < activePointLights) {
vec3 center = uPointLightPos[chosen];
float radius = max(uPointLightRadius[chosen], 0.05);
vec3 toCenter = center - pos;
float dist = length(toCenter);
if(dist < radius + 1e-4) {
guidedDir = normalize(sampleUniformSphere(rngState));
return true;
}

float sinThetaMax = clamp(radius / max(dist, 1e-4), 0.0, 0.9999);
float cosThetaMax = sqrt(max(0.0, 1.0 - sinThetaMax * sinThetaMax));
guidedDir = sampleUniformCone(normalize(toCenter), cosThetaMax, rngState);
return true;
}

vec3 sunAxis = normalize(lightDir);
float sunCosThetaMax = cos(radians(1.0));
guidedDir = sampleUniformCone(sunAxis, sunCosThetaMax, rngState);
return true;
}

float cosineHemispherePdf(vec3 n, vec3 dir) {
return max(dot(normalize(n), normalize(dir)), 0.0) / PI;
}

float uniformConePdf(float cosThetaMax) {
return 1.0 / max(2.0 * PI * (1.0 - cosThetaMax), 1e-6);
}

float pointLightDirectionalPdf(vec3 pos, vec3 dir, int pointLightIndex) {
vec3 center = uPointLightPos[pointLightIndex];
float radius = max(uPointLightRadius[pointLightIndex], 0.05);
vec3 toCenter = center - pos;
float dist = length(toCenter);
if(dist <= radius + 1e-4) {
return 1.0 / (4.0 * PI);
}

vec3 axis = toCenter / max(dist, 1e-6);
float sinThetaMax = clamp(radius / max(dist, 1e-4), 0.0, 0.9999);
float cosThetaMax = sqrt(max(0.0, 1.0 - sinThetaMax * sinThetaMax));
if(dot(normalize(dir), axis) < cosThetaMax) {
return 0.0;
}

return uniformConePdf(cosThetaMax);
}

float sunDirectionalPdf(vec3 dir) {
if(max(directLightIntensity, 0.0) <= 1e-5) {
return 0.0;
}

float sunCosThetaMax = cos(radians(1.0));
if(dot(normalize(dir), normalize(lightDir)) < sunCosThetaMax) {
return 0.0;
}
return uniformConePdf(sunCosThetaMax);
}

float guidedDirectionalPdf(vec3 pos, vec3 dir) {
int activePointLights = min(max(uPointLightCount, 0), MAX_POINT_LIGHTS);
bool hasSun = max(directLightIntensity, 0.0) > 1e-5;
int candidateCount = activePointLights + (hasSun ? 1 : 0);
if(candidateCount <= 0) {
return 0.0;
}

float pdfSum = 0.0;
for(int pointLightIndex = 0;
pointLightIndex < activePointLights;
++ pointLightIndex) {
pdfSum += pointLightDirectionalPdf(pos, dir, pointLightIndex);
}
if(hasSun) {
pdfSum += sunDirectionalPdf(dir);
}

return pdfSum / float(candidateCount);
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

for(int bounce = 0;
bounce < MAX_BOUNCES;
++ bounce) {
if(bounce >= bounceLimit) {
break;
}

float pointLightHitT;
vec3 pointLightEmission;
bool hitPointLight = intersectPointLightEmitters(ro, rd, MAX_DIST, pointLightHitT, pointLightEmission);

HitInfo hit;
bool hitScene = marchScene(ro, rd, hit);
if(! hitScene) {
if(hitPointLight) {
radiance += throughput * pointLightEmission;
break;
}
radiance += throughput * sampleSky(rd);
break;
}

float sceneHitT = length(hit.position - ro);
if(hitPointLight && pointLightHitT < sceneHitT) {
radiance += throughput * pointLightEmission;
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
vec3 N = frontFace ? outwardNormal : - outwardNormal;

vec3 F0 = mix(vec3(0.04), albedo, metallic);
float NdotV = max(dot(N, - rd), 0.0);

float etaI = frontFace ? 1.0 : max(ior, 1.0001);
float etaT = frontFace ? max(ior, 1.0001) : 1.0;
float eta = etaI / etaT;
float cosTheta = clamp(dot(- rd, N), 0.0, 1.0);
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
} else if(chooser < reflectProb + refractProb) {
vec3 refracted = refract(rd, N, eta);
if(dot(refracted, refracted) < 1e-6) {
refracted = reflect(rd, N);
}
vec3 refractedGloss = sampleCosineHemisphere(refracted, rngState);
nextDir = normalize(mix(refracted, refractedGloss, roughness * roughness * 0.35));
vec3 transTint = mix(vec3(1.0), albedo, 0.2);
throughput *= transTint / max(refractProb, 0.001);
} else {
vec3 diffuseDir = sampleCosineHemisphere(N, rngState);
vec3 guidedDir;
bool hasGuided = sampleLightGuidedDirection(hit.position + N * (SURFACE_DIST * 6.0), rngState, guidedDir);

float guidedMixProb = 0.2;
bool useGuidedSampler = hasGuided && dot(guidedDir, N) > 0.0;

if(useGuidedSampler) {
if(rand01(rngState) < guidedMixProb) {
nextDir = guidedDir;
} else {
nextDir = diffuseDir;
}
} else {
nextDir = diffuseDir;
}

vec3 diffuseTint = (vec3(1.0) - Fv) * (1.0 - metallic) * (1.0 - transmission) * albedo;
float cosinePdf = cosineHemispherePdf(N, nextDir);
float guidedPdf = useGuidedSampler ? guidedDirectionalPdf(hit.position + N * (SURFACE_DIST * 6.0), nextDir) : 0.0;
float effectiveGuideProb = useGuidedSampler ? guidedMixProb : 0.0;
float mixedPdf = (1.0 - effectiveGuideProb) * cosinePdf + effectiveGuideProb * guidedPdf;
float diffuseMis = cosinePdf / max(mixedPdf, 1e-6);
throughput *= diffuseTint * diffuseMis / max(diffuseProb, 0.001);
}

throughput = max(throughput, vec3(0.0));

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

return max(radiance, vec3(0.0));
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
