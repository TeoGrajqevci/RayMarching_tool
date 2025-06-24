#version 330 core
out vec4 FragColor;
in vec2 fragCoord;
uniform vec2 iResolution;
uniform float iTime;
// ----- Material and SDF Result Structures -----
struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
};
struct SDFResult {
    float dist;
    Material mat;
};
// --- New uniform arrays for PBR parameters ---
uniform float shapeMetallic[50];
uniform float shapeRoughness[50];
uniform vec3 shapeAlbedos[50];
// --- Scene shape uniforms ---
uniform int shapeCount;
uniform int shapeTypes[50];       // 0: sphere, 1: box, 2: round box, 3: torus, 4: cylinder
uniform vec3 shapeCenters[50];
uniform vec4 shapeParams[50];
uniform vec3 shapeRotations[50];
uniform vec3 shapeScales[50];     // Non-uniform scaling per shape
// NEW: Blend mode parameters per shape
uniform float shapeSmoothness[50];
uniform int shapeBlendOp[50];     // 0: None, 1: Smooth Union, 2: Smooth Subtraction, 3: Smooth Intersection
// --- Selected shapes uniforms (for outline raymarching) ---
uniform int uSelectedCount;
uniform int uSelectedTypes[50];
uniform vec3 uSelectedCenters[50];
uniform vec4 uSelectedParams[50];
uniform vec3 uSelectedRotations[50];
uniform vec3 uSelectedScales[50];
uniform float uOutlineThickness;
// --- Lighting uniforms ---
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform int uRenderMode;
// --- Camera uniforms ---
uniform vec3 cameraPos;
uniform vec3 cameraTarget;
// --- Background toggle ---
uniform int uBackgroundGradient;

// --- Axis Guide uniforms ---
uniform int uShowAxisGuides;    // 0 ou 1 pour afficher les guides d'axe
uniform int uActiveAxis;        // -1: aucun, 0: X, 1: Y, 2: Z
uniform vec3 uGuideCenter;      // Centre du guide (centre de la forme sélectionnée)
// ----- Constants -----
const int MAX_STEPS = 100;
const float MAX_DIST = 100.0;
const float SURFACE_DIST = 0.001;
// ----- Blend Operation Functions for SDFResult -----
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
// ----- Helper: World-Space Rotation Functions -----
vec3 rotateX(vec3 p, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec3(p.x, c * p.y - s * p.z, s * p.y + c * p.z);
}

vec3 rotateY(vec3 p, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec3(c * p.x + s * p.z, p.y, -s * p.x + c * p.z);
}

vec3 rotateZ(vec3 p, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec3(c * p.x - s * p.y, s * p.x + c * p.y, p.z);
}

// World-space rotation: apply rotations in world-space order (X, Y, Z)
// This ensures that rotations are applied around world axes
vec3 rotateWorld(vec3 p, vec3 angles) {
    vec3 result = p;
    // Apply rotations in world-space order: X, then Y, then Z
    // Each rotation is applied around the original world axes
    if (angles.x != 0.0) result = rotateX(result, angles.x);
    if (angles.y != 0.0) result = rotateY(result, angles.y);
    if (angles.z != 0.0) result = rotateZ(result, angles.z);
    return result;
}

// Keep the old rotate function for backward compatibility but using world-space rotations
vec3 rotate(vec3 p, vec3 angles) {
    return rotateWorld(p, angles);
}
// ----- Basic SDF Functions -----
float sdSphere(vec3 p, vec3 center, float radius) {
    return length(p - center) - radius;
}

// Ellipsoid SDF - allows non-uniform scaling
float sdEllipsoid(vec3 p, vec3 center, vec3 radii) {
    vec3 localP = p - center;
    vec3 scaledP = localP / radii;
    return (length(scaledP) - 1.0) * min(min(radii.x, radii.y), radii.z);
}

// Ellipsoid with rotation (now using world-space rotation)
float sdEllipsoidRotated(vec3 p, vec3 center, vec3 radii, vec3 rotation) {
    vec3 localP = p - center;
    localP = rotateWorld(localP, rotation); // Use positive rotation for world-space
    vec3 scaledP = localP / radii; // Puis la mise à l'échelle dans l'espace local
    return (length(scaledP) - 1.0) * min(min(radii.x, radii.y), radii.z);
}
float sdBoxRotated(vec3 p, vec3 halfExtents, vec3 rotation) {
    vec3 localP = rotateWorld(p, rotation); // Use positive rotation for world-space
    vec3 d = abs(localP) - halfExtents;
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

// Box with non-uniform scaling (now using world-space rotation)
float sdBoxScaled(vec3 p, vec3 center, vec3 halfExtents, vec3 scale, vec3 rotation) {
    vec3 localP = p - center;
    localP = rotateWorld(localP, rotation); // Use positive rotation for world-space
    localP = localP / scale; // Puis la mise à l'échelle dans l'espace local
    vec3 d = abs(localP) - halfExtents;
    float boxDist = length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
    return boxDist * min(min(scale.x, scale.y), scale.z);
}
float sdRoundBox(vec3 p, vec3 b, float r) {
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) - r + min(max(q.x, max(q.y, q.z)), 0.0);
}
float sdRoundBoxRotated(vec3 p, vec3 b, float r, vec3 rotation) {
    vec3 localP = rotateWorld(p, rotation); // Use positive rotation for world-space
    return sdRoundBox(localP, b, r);
}

// Round box with non-uniform scaling (now using world-space rotation)
float sdRoundBoxScaled(vec3 p, vec3 center, vec3 halfExtents, float r, vec3 scale, vec3 rotation) {
    vec3 localP = p - center;
    localP = rotateWorld(localP, rotation); // Use positive rotation for world-space
    localP = localP / scale; // Puis la mise à l'échelle dans l'espace local
    float roundBoxDist = sdRoundBox(localP, halfExtents, r / min(min(scale.x, scale.y), scale.z));
    return roundBoxDist * min(min(scale.x, scale.y), scale.z);
}
float sdTorus(vec3 p, vec2 t) {
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}
float sdTorusRotated(vec3 p, vec2 t, vec3 rotation) {
    vec3 localP = rotateWorld(p, rotation); // Use positive rotation for world-space
    return sdTorus(localP, t);
}

// Torus with non-uniform scaling (creates elliptical torus, now using world-space rotation)
float sdTorusScaled(vec3 p, vec3 center, vec2 t, vec3 scale, vec3 rotation) {
    vec3 localP = p - center;
    localP = rotateWorld(localP, rotation); // Use positive rotation for world-space
    localP = localP / scale; // Puis la mise à l'échelle dans l'espace local
    // For torus, we need to adjust the radii based on XZ scaling for the major radius
    // and Y scaling affects the tube radius
    vec2 adjustedT = vec2(t.x / min(scale.x, scale.z), t.y / scale.y);
    float torusDist = sdTorus(localP, adjustedT);
    return torusDist * min(min(scale.x, scale.y), scale.z);
}
float sdCylinder(vec3 p, float r, float h) {
    vec2 d = vec2(length(p.xz) - r, abs(p.y) - h);
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}
float sdCylinderRotated(vec3 p, float r, float h, vec3 rotation) {
    vec3 localP = rotateWorld(p, rotation); // Use positive rotation for world-space
    return sdCylinder(localP, r, h);
}

// Cylinder with non-uniform scaling (creates elliptical cylinder, now using world-space rotation)
float sdCylinderScaled(vec3 p, vec3 center, float r, float h, vec3 scale, vec3 rotation) {
    vec3 localP = p - center;
    localP = rotateWorld(localP, rotation); // Use positive rotation for world-space
    localP = localP / scale; // Puis la mise à l'échelle dans l'espace local
    // For cylinder, XZ scaling affects the radius, Y scaling affects the height
    float adjustedR = r / min(scale.x, scale.z);
    float adjustedH = h / scale.y;
    float cylinderDist = sdCylinder(localP, adjustedR, adjustedH);
    return cylinderDist * min(min(scale.x, scale.y), scale.z);
}
// ----- Acceleration Helper: Bounding Radius per Shape -----
float shapeBoundingRadius(int i) {
    vec3 scale = shapeScales[i];
    float maxScale = max(max(scale.x, scale.y), scale.z);
    
    if (shapeTypes[i] == 0) { // Sphere/Ellipsoid
         return shapeParams[i].w * maxScale;
    } else if (shapeTypes[i] == 1) { // Box
         return length(shapeParams[i].xyz) * maxScale;
    } else if (shapeTypes[i] == 2) { // Round Box
         return (length(shapeParams[i].xyz) + shapeParams[i].w) * maxScale;
    } else if (shapeTypes[i] == 3) { // Torus
         return (shapeParams[i].x + shapeParams[i].y) * maxScale;
    } else if (shapeTypes[i] == 4) { // Cylinder
         return sqrt(shapeParams[i].x * shapeParams[i].x + shapeParams[i].y * shapeParams[i].y) * maxScale;
    }
    return MAX_DIST;
}
// ----- SDFResult Helper for a Given Shape -----
SDFResult sdfForShape(int i, vec3 p) {
    SDFResult res;
    float d = MAX_DIST;
    vec3 scale = shapeScales[i];
    
    if (shapeTypes[i] == 0) { // Sphere -> Ellipsoid
         d = sdEllipsoidRotated(p, shapeCenters[i], vec3(shapeParams[i].w) * scale, shapeRotations[i]);
    } else if (shapeTypes[i] == 1) { // Box with scaling
         d = sdBoxScaled(p, shapeCenters[i], shapeParams[i].xyz, scale, shapeRotations[i]);
    } else if (shapeTypes[i] == 2) { // Round Box with scaling
         d = sdRoundBoxScaled(p, shapeCenters[i], shapeParams[i].xyz, shapeParams[i].w, scale, shapeRotations[i]);
    } else if (shapeTypes[i] == 3) { // Torus with scaling
         d = sdTorusScaled(p, shapeCenters[i], vec2(shapeParams[i].x, shapeParams[i].y), scale, shapeRotations[i]);
    } else if (shapeTypes[i] == 4) { // Cylinder with scaling
         d = sdCylinderScaled(p, shapeCenters[i], shapeParams[i].x, shapeParams[i].y, scale, shapeRotations[i]);
    }
    res.dist = d;
    res.mat.albedo = shapeAlbedos[i];
    res.mat.metallic = shapeMetallic[i];
    res.mat.roughness = shapeRoughness[i];
    return res;
}
// ----- Scene SDF with Material Blending -----
SDFResult sceneSDF(vec3 p) {
    SDFResult res;
    res.dist = MAX_DIST;
    res.mat.albedo = vec3(0.0);
    for (int i = 0; i < shapeCount; i++) {
         SDFResult candidate = sdfForShape(i, p);
         if(shapeBlendOp[i] == 0) {
             float boundDist = length(p - shapeCenters[i]) - shapeBoundingRadius(i);
             if(boundDist > res.dist) {
                 continue;
             }
         }
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
// Estimate surface normal via finite differences.
vec3 getNormal(vec3 p) {
    float eps = 0.001;
    return normalize(vec3(
       sceneSDF(p + vec3(eps, 0, 0)).dist - sceneSDF(p - vec3(eps, 0, 0)).dist,
       sceneSDF(p + vec3(0, eps, 0)).dist - sceneSDF(p - vec3(0, eps, 0)).dist,
       sceneSDF(p + vec3(0, 0, eps)).dist - sceneSDF(p - vec3(0, 0, eps)).dist
    ));
}
// ----- Soft Shadows Function -----
float softShadow(vec3 ro, vec3 rd, float mint, float maxt, float k) {
    float res = 1.0;
    float t = mint;
    for (int i = 0; i < 32; i++) {
         float h = sceneSDF(ro + rd * t).dist;
         if (h < SURFACE_DIST)
             return 0.0;
         res = min(res, k * h / t);
         t += h;
         if (t >= maxt) break;
    }
    return res;
}
// ----- Selected Shape SDF Helper for given index (for edge detection) -----
float selectedShapeSDFFor(int idx, vec3 p) {
    float d = MAX_DIST;
    vec3 scale = uSelectedScales[idx];
    
    if (uSelectedTypes[idx] == 0) { // Sphere -> Ellipsoid
         d = sdEllipsoidRotated(p, uSelectedCenters[idx], vec3(uSelectedParams[idx].w) * scale, uSelectedRotations[idx]);
    } else if (uSelectedTypes[idx] == 1) { // Box with scaling
         d = sdBoxScaled(p, uSelectedCenters[idx], uSelectedParams[idx].xyz, scale, uSelectedRotations[idx]);
    } else if (uSelectedTypes[idx] == 2) { // Round Box with scaling
         d = sdRoundBoxScaled(p, uSelectedCenters[idx], uSelectedParams[idx].xyz, uSelectedParams[idx].w, scale, uSelectedRotations[idx]);
    } else if (uSelectedTypes[idx] == 3) { // Torus with scaling
         d = sdTorusScaled(p, uSelectedCenters[idx], vec2(uSelectedParams[idx].x, uSelectedParams[idx].y), scale, uSelectedRotations[idx]);
    } else if (uSelectedTypes[idx] == 4) { // Cylinder with scaling
         d = sdCylinderScaled(p, uSelectedCenters[idx], uSelectedParams[idx].x, uSelectedParams[idx].y, scale, uSelectedRotations[idx]);
    }
    return d;
}
// ----- Raymarching for Selected Shape Edge Detection for given index -----
vec4 rayMarchSelectedFor(int idx, vec3 ro, vec3 rd) {
    const float maxDist = MAX_DIST;
    const float minDist = SURFACE_DIST;
    const int maxIter = 40;
    float dist = 0.0;
    float lastD = 1e10;
    float edge = 0.0;
    for (int i = 0; i < maxIter; i++) {
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
         if (dist > maxDist) break;
    }
    return vec4(dist, 0.0, edge, 0.0);
}
// ----- AGX Tone Mapping Function -----
vec3 AgXToneMapping(vec3 color) {
    color *= 1.0;
    vec3 aces = clamp((color * (color + 0.0245786) - 0.000090537) /
                      (color * (0.983729 * color + 0.4329510) + 0.238081),
                      0.0, 1.0);
    vec3 result = mix(aces, color, 0.35);
    return result;
}

// ----- Axis Guide Line SDF -----
float sdAxisLine(vec3 p, vec3 center, int axis) {
    vec3 localP = p - center;
    if (axis == 0) { // X axis
        return length(localP.yz);
    } else if (axis == 1) { // Y axis
        return length(localP.xz);
    } else if (axis == 2) { // Z axis
        return length(localP.xy);
    }
    return MAX_DIST;
}

// ----- Raymarching for Axis Guide Lines -----
vec4 rayMarchAxisGuide(vec3 ro, vec3 rd, vec3 center, int axis) {
    const float maxDist = MAX_DIST;
    const float minDist = 0.002;
    const int maxIter = 64;
    float dist = 0.0;
    float lineWidth = 0.002; // Épaisseur de la ligne
    
    for (int i = 0; i < maxIter; i++) {
        vec3 pos = ro + rd * dist;
        float d = sdAxisLine(pos, center, axis) - lineWidth;
        
        if (d < minDist) {
            return vec4(dist, 0.0, 1.0, 0.0); // Ligne trouvée
        }
        
        dist += d;
        if (dist > maxDist) break;
    }
    
    return vec4(maxDist, 0.0, 0.0, 0.0); // Pas de ligne
}

// ----- Modified Main Image Function using a basic PBR model -----
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord * 0.5 + 0.5;
    vec2 pScreen = (uv - 0.5) * 2.0;
    pScreen.x *= iResolution.x / iResolution.y;
    vec3 forward = normalize(cameraTarget - cameraPos);
    vec3 right = normalize(cross(forward, vec3(0.0, 1.0, 0.0)));
    vec3 up = cross(right, forward);
    vec3 ro = cameraPos;
    vec3 rd = normalize(forward + pScreen.x * right + pScreen.y * up);
    vec3 hitPos;
    float dist = 0.0;
    for (int i = 0; i < MAX_STEPS; i++) {
        hitPos = ro + rd * dist;
        float dScene = sceneSDF(hitPos).dist;
        if (dScene < SURFACE_DIST) {
            break;
        }
        dist += dScene;
        if (dist > MAX_DIST) break;
    }
    vec3 color = vec3(0.0);
    if (dist < MAX_DIST) {
         vec3 N = normalize(getNormal(hitPos));
         vec3 V = normalize(cameraPos - hitPos);
         SDFResult hitRes = sceneSDF(hitPos);
         vec3 baseColor = hitRes.mat.albedo;
         float metallic = hitRes.mat.metallic;
         float roughness = hitRes.mat.roughness;
         vec3 F0 = mix(vec3(0.04), baseColor, metallic);
         vec3 L = normalize(lightDir);
         float NdotL = max(dot(N, L), 0.0);
         vec3 diffuse = baseColor * (1.0 - metallic) * NdotL * lightColor;
         vec3 H = normalize(L + V);
         float NdotH = max(dot(N, H), 0.0);
         float specular = pow(NdotH, 1.0/(roughness+0.001));
         vec3 spec = F0 * specular * lightColor;
         
         // Calculate shadow for direct lighting only
         float shadow = softShadow(hitPos + N*SURFACE_DIST*2.0, L, 0.001, MAX_DIST, 32.0);
         
         // Apply shadow only to direct lighting, keep ambient lighting always present
         vec3 directLighting = (diffuse + spec) * shadow;
         vec3 ambientLighting = ambientColor * baseColor;
         color = directLighting + ambientLighting;
    } else {
         vec3 baseColor = (uBackgroundGradient==1) ? vec3(0.1) : vec3(0.0);
         color = baseColor + ambientColor ;
    }
    
    if(uRenderMode == 0)
    {
        if (uSelectedCount > 0) {
             float combinedEdge = 0.0;
             for (int i = 0; i < uSelectedCount; i++) {
                 vec4 selEdge = rayMarchSelectedFor(i, ro, rd);
                 combinedEdge = max(combinedEdge, selEdge.z);
             }
             if (combinedEdge > 0.0) {
                 vec3 outlineColor = vec3(1.0, 0.5, 0.0);
                 color = mix(color, outlineColor, combinedEdge);
             }
        }
        
        // Rendu des guides d'axe
        if (uShowAxisGuides == 1 && uActiveAxis >= 0) {
            vec4 axisGuide = rayMarchAxisGuide(ro, rd, uGuideCenter, uActiveAxis);
            if (axisGuide.z > 0.0) {
                vec3 axisColor = vec3(1.0, 0.0, 0.0); // Rouge par défaut
                if (uActiveAxis == 0) axisColor = vec3(1.0, 0.0, 0.0);      // X = Rouge
                else if (uActiveAxis == 1) axisColor = vec3(0.0, 0.0, 1.0); // Y = Bleu
                else if (uActiveAxis == 2) axisColor = vec3(0.0, 1.0, 0.0); // Z = Vert
                
                // Mélanger avec la couleur existante
                float axisIntensity = 0.4;
                color = mix(color, axisColor, axisIntensity);
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