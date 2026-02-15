#version 330 core

out vec4 FragColor;
in vec2 fragCoord;

uniform sampler2D uTexture;
uniform vec2 uResolution;
uniform vec3 uCameraPos;
uniform vec3 uCameraTarget;
uniform float uCameraFovDegrees;
uniform int uCameraProjectionMode;
uniform float uCameraOrthoScale;
uniform int uMirrorHelperShow;
uniform vec3 uMirrorHelperAxisEnabled;
uniform vec3 uMirrorHelperOffset;
uniform vec3 uMirrorHelperCenter;
uniform float uMirrorHelperExtent;
uniform int uMirrorHelperSelected;
uniform int uMirrorHelperSelectedAxis;

vec3 AgXToneMapping(vec3 color) {
    vec3 aces = clamp((color * (color + 0.0245786) - 0.000090537) /
                      (color * (0.983729 * color + 0.4329510) + 0.238081),
                      0.0, 1.0);
    return mix(aces, color, 0.35);
}

void computeDisplayRay(out vec3 ro, out vec3 rd) {
    vec2 uv = fragCoord * 0.5 + 0.5;
    vec2 pScreen = (uv - 0.5) * 2.0;
    pScreen.x *= uResolution.x / max(uResolution.y, 1.0);

    vec3 forward = normalize(uCameraTarget - uCameraPos);
    vec3 right = normalize(cross(forward, vec3(0.0, 1.0, 0.0)));
    if (length(right) < 1e-5) {
        right = vec3(1.0, 0.0, 0.0);
    }
    vec3 up = normalize(cross(right, forward));

    if (uCameraProjectionMode == 1) {
        float orthoScale = max(uCameraOrthoScale, 1e-4);
        ro = uCameraPos + (pScreen.x * right + pScreen.y * up) * orthoScale;
        rd = forward;
        return;
    }

    float tanHalfFov = tan(radians(clamp(uCameraFovDegrees, 1.0, 179.0)) * 0.5);
    vec2 perspectiveOffset = pScreen * tanHalfFov;
    ro = uCameraPos;
    rd = normalize(forward + perspectiveOffset.x * right + perspectiveOffset.y * up);
}

vec3 applyMirrorHelperOverlay(vec3 color) {
    if (uMirrorHelperShow != 1) {
        return color;
    }

    vec3 ro;
    vec3 rd;
    computeDisplayRay(ro, rd);

    float helperExtent = max(uMirrorHelperExtent, 0.1);
    float helperLineWidth = max(0.03 * helperExtent, 0.03);
    float helperGridSpacing = max(helperExtent * 0.25, 0.25);

    float nearestHelperT = 1e20;
    vec3 nearestHelperColor = vec3(0.0);
    float nearestHelperAlpha = 0.0;

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
        if (tPlane <= 0.0 || tPlane >= nearestHelperT) {
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
    return color;
}

void main() {
    vec2 uv = fragCoord * 0.5 + 0.5;
    vec3 color = texture(uTexture, uv).rgb;
    color = max(color, vec3(0.0));
    color = AgXToneMapping(color);
    color = pow(color, vec3(1.0 / 2.2));
    color = applyMirrorHelperOverlay(color);
    FragColor = vec4(color, 1.0);
}
