#include "rmt/app/benchmark/BenchmarkScenes.h"

#include "rmt/common/math/SphericalCoordinates.h"

#include <algorithm>
#include <cmath>

namespace rmt {

namespace {

Shape makeBaseShape(int type, const std::string& name) {
    Shape shape;
    shape.type = type;
    shape.center[0] = shape.center[1] = shape.center[2] = 0.0f;
    shape.param[0] = 0.5f;
    shape.param[1] = 0.5f;
    shape.param[2] = 0.5f;
    shape.fractalExtra[0] = 1.0f;
    shape.fractalExtra[1] = 1.0f;
    shape.rotation[0] = shape.rotation[1] = shape.rotation[2] = 0.0f;
    shape.extra = 0.0f;
    shape.scale[0] = shape.scale[1] = shape.scale[2] = 1.0f;
    shape.scaleMode = SCALE_MODE_ELONGATE;
    shape.elongation[0] = shape.elongation[1] = shape.elongation[2] = 0.0f;
    shape.twist[0] = shape.twist[1] = shape.twist[2] = 0.0f;
    shape.bend[0] = shape.bend[1] = shape.bend[2] = 0.0f;
    shape.name = name;
    shape.blendOp = BLEND_NONE;
    shape.smoothness = 0.08f;
    shape.albedo[0] = 0.85f;
    shape.albedo[1] = 0.86f;
    shape.albedo[2] = 0.88f;
    shape.metallic = 0.0f;
    shape.roughness = 0.08f;
    shape.emission[0] = 0.0f;
    shape.emission[1] = 0.0f;
    shape.emission[2] = 0.0f;
    shape.emissionStrength = 0.0f;
    shape.transmission = 0.0f;
    shape.ior = 1.5f;
    shape.bendModifierEnabled = false;
    shape.twistModifierEnabled = false;
    shape.mirrorModifierEnabled = false;
    shape.mirrorAxis[0] = false;
    shape.mirrorAxis[1] = false;
    shape.mirrorAxis[2] = false;
    shape.mirrorOffset[0] = 0.0f;
    shape.mirrorOffset[1] = 0.0f;
    shape.mirrorOffset[2] = 0.0f;
    shape.mirrorSmoothness = 0.0f;
    return shape;
}

std::vector<Shape> buildSimplePrimitivesCsgBenchmarkScene() {
    std::vector<Shape> shapes;
    shapes.reserve(18);

    Shape floor = makeBaseShape(SHAPE_BOX, "floor");
    floor.param[0] = 4.5f;
    floor.param[1] = 0.22f;
    floor.param[2] = 4.5f;
    floor.center[1] = -1.25f;
    floor.albedo[0] = 0.70f;
    floor.albedo[1] = 0.72f;
    floor.albedo[2] = 0.76f;
    floor.roughness = 0.18f;
    shapes.push_back(floor);

    Shape sphere = makeBaseShape(SHAPE_SPHERE, "main_sphere");
    sphere.center[0] = -0.8f;
    sphere.center[1] = -0.1f;
    sphere.param[0] = 0.9f;
    sphere.albedo[0] = 0.95f;
    sphere.albedo[1] = 0.55f;
    sphere.albedo[2] = 0.38f;
    shapes.push_back(sphere);

    Shape cutter = makeBaseShape(SHAPE_BOX, "cutter_box");
    cutter.center[0] = -0.55f;
    cutter.center[1] = -0.1f;
    cutter.param[0] = 0.32f;
    cutter.param[1] = 0.95f;
    cutter.param[2] = 0.32f;
    cutter.rotation[2] = 0.45f;
    cutter.blendOp = BLEND_SMOOTH_SUBTRACTION;
    cutter.smoothness = 0.08f;
    shapes.push_back(cutter);

    Shape torus = makeBaseShape(SHAPE_TORUS, "main_torus");
    torus.center[0] = 1.0f;
    torus.center[1] = 0.1f;
    torus.param[0] = 0.8f;
    torus.param[1] = 0.22f;
    torus.rotation[0] = 0.35f;
    torus.albedo[0] = 0.45f;
    torus.albedo[1] = 0.86f;
    torus.albedo[2] = 0.95f;
    shapes.push_back(torus);

    Shape cap = makeBaseShape(SHAPE_SPHERE, "torus_cap");
    cap.center[0] = 1.0f;
    cap.center[1] = 0.1f;
    cap.param[0] = 0.62f;
    cap.blendOp = BLEND_SMOOTH_INTERSECTION;
    cap.smoothness = 0.10f;
    cap.albedo[0] = 0.45f;
    cap.albedo[1] = 0.86f;
    cap.albedo[2] = 0.95f;
    shapes.push_back(cap);

    Shape detailA = makeBaseShape(SHAPE_CYLINDER, "detail_a");
    detailA.center[0] = -1.9f;
    detailA.center[1] = -0.45f;
    detailA.center[2] = 0.9f;
    detailA.param[0] = 0.22f;
    detailA.param[1] = 0.55f;
    detailA.rotation[0] = 0.45f;
    detailA.rotation[2] = 0.15f;
    detailA.albedo[0] = 0.86f;
    detailA.albedo[1] = 0.75f;
    detailA.albedo[2] = 0.95f;
    shapes.push_back(detailA);

    Shape detailB = makeBaseShape(SHAPE_CONE, "detail_b");
    detailB.center[0] = 2.0f;
    detailB.center[1] = -0.35f;
    detailB.center[2] = -0.6f;
    detailB.param[0] = 0.36f;
    detailB.param[1] = 0.72f;
    detailB.albedo[0] = 0.84f;
    detailB.albedo[1] = 0.92f;
    detailB.albedo[2] = 0.62f;
    shapes.push_back(detailB);

    return shapes;
}

std::vector<Shape> buildRepeatedGeometryBenchmarkScene() {
    const int repeatCount = 20 * 14;
    std::vector<Shape> shapes;
    shapes.reserve(static_cast<std::size_t>(repeatCount + 1));

    Shape floor = makeBaseShape(SHAPE_BOX, "floor");
    floor.param[0] = 7.0f;
    floor.param[1] = 0.15f;
    floor.param[2] = 7.0f;
    floor.center[1] = -1.5f;
    floor.albedo[0] = 0.66f;
    floor.albedo[1] = 0.68f;
    floor.albedo[2] = 0.71f;
    floor.roughness = 0.2f;
    shapes.push_back(floor);

    const int cols = 20;
    const int rows = 14;
    const float spacing = 0.72f;
    const float cx = 0.5f * static_cast<float>(cols - 1);
    const float cz = 0.5f * static_cast<float>(rows - 1);
    int idx = 0;
    for (int z = 0; z < rows; ++z) {
        for (int x = 0; x < cols; ++x) {
            Shape shape =
                makeBaseShape((idx % 2 == 0) ? SHAPE_SPHERE : SHAPE_TORUS,
                              std::string("rep_") + std::to_string(idx));
            shape.center[0] = (static_cast<float>(x) - cx) * spacing;
            shape.center[1] = -0.85f +
                              0.06f *
                                  std::sin(static_cast<float>(x) * 0.6f +
                                           static_cast<float>(z) * 0.45f);
            shape.center[2] = (static_cast<float>(z) - cz) * spacing;
            shape.rotation[1] = 0.1f * static_cast<float>((x + z) % 9);
            if (shape.type == SHAPE_SPHERE) {
                shape.param[0] = 0.26f;
                shape.albedo[0] = 0.90f;
                shape.albedo[1] = 0.66f;
                shape.albedo[2] = 0.48f;
            } else {
                shape.param[0] = 0.27f;
                shape.param[1] = 0.08f;
                shape.albedo[0] = 0.48f;
                shape.albedo[1] = 0.82f;
                shape.albedo[2] = 0.95f;
            }
            if ((idx % 11) == 0) {
                shape.blendOp = BLEND_SMOOTH_SUBTRACTION;
                shape.smoothness = 0.05f;
                shape.param[0] *= 0.65f;
            } else if ((idx % 7) == 0) {
                shape.blendOp = BLEND_SMOOTH_UNION;
                shape.smoothness = 0.07f;
            }
            shapes.push_back(shape);
            ++idx;
        }
    }

    return shapes;
}

std::vector<Shape> buildWorstCaseThinFeatureBenchmarkScene(int ringCount = 160,
                                                           int fractalCount = 28) {
    std::vector<Shape> shapes;
    ringCount = std::max(8, ringCount);
    fractalCount = std::max(1, fractalCount);
    shapes.reserve(static_cast<std::size_t>(2 + ringCount + fractalCount));

    Shape floor = makeBaseShape(SHAPE_BOX, "floor");
    floor.param[0] = 6.2f;
    floor.param[1] = 0.12f;
    floor.param[2] = 6.2f;
    floor.center[1] = -1.15f;
    floor.albedo[0] = 0.72f;
    floor.albedo[1] = 0.73f;
    floor.albedo[2] = 0.75f;
    floor.roughness = 0.25f;
    shapes.push_back(floor);

    for (int i = 0; i < ringCount; ++i) {
        const float fi = static_cast<float>(i);
        Shape ring = makeBaseShape(SHAPE_TORUS, std::string("thin_torus_") + std::to_string(i));
        ring.param[0] = 0.35f + 0.05f * std::sin(fi * 0.19f);
        ring.param[1] = 0.028f + 0.01f * std::sin(fi * 0.53f);
        ring.center[0] = std::sin(fi * 0.31f) * 2.4f;
        ring.center[1] = -0.35f + 0.75f * std::sin(fi * 0.17f);
        ring.center[2] = std::cos(fi * 0.29f) * 2.1f;
        ring.rotation[0] = fi * 0.09f;
        ring.rotation[1] = fi * 0.07f;
        ring.rotation[2] = fi * 0.05f;
        ring.albedo[0] = 0.55f + 0.35f * std::sin(fi * 0.11f);
        ring.albedo[1] = 0.60f + 0.30f * std::sin(fi * 0.13f + 1.0f);
        ring.albedo[2] = 0.62f + 0.28f * std::sin(fi * 0.17f + 2.0f);
        ring.roughness = 0.05f + 0.10f * (std::sin(fi * 0.23f) * 0.5f + 0.5f);
        if ((i % 9) == 0) {
            ring.blendOp = BLEND_SMOOTH_INTERSECTION;
            ring.smoothness = 0.03f;
        } else if ((i % 7) == 0) {
            ring.blendOp = BLEND_SMOOTH_SUBTRACTION;
            ring.smoothness = 0.04f;
        } else if ((i % 5) == 0) {
            ring.blendOp = BLEND_SMOOTH_UNION;
            ring.smoothness = 0.03f;
        }
        if ((i % 8) == 0) {
            ring.transmission = 0.7f;
            ring.ior = 1.42f;
            ring.roughness = 0.03f;
        }
        shapes.push_back(ring);
    }

    for (int i = 0; i < fractalCount; ++i) {
        const float fi = static_cast<float>(i);
        Shape fractal = makeBaseShape(SHAPE_MANDELBULB, std::string("fractal_") + std::to_string(i));
        fractal.center[0] = -2.8f + fi * 0.22f;
        fractal.center[1] = -0.65f + 0.1f * std::sin(fi * 0.9f);
        fractal.center[2] = -1.6f + 0.13f * std::cos(fi * 0.6f);
        fractal.param[0] = 8.0f;
        fractal.param[1] = 7.0f;
        fractal.param[2] = 3.0f;
        fractal.fractalExtra[0] = 1.2f;
        fractal.fractalExtra[1] = 1.1f;
        fractal.scale[0] = fractal.scale[1] = fractal.scale[2] = 0.38f;
        fractal.scaleMode = SCALE_MODE_DEFORM;
        fractal.albedo[0] = 0.95f;
        fractal.albedo[1] = 0.58f;
        fractal.albedo[2] = 0.26f;
        fractal.roughness = 0.08f;
        if (i % 2 == 0) {
            fractal.blendOp = BLEND_SMOOTH_UNION;
            fractal.smoothness = 0.05f;
        }
        shapes.push_back(fractal);
    }

    return shapes;
}

} // namespace

std::vector<Shape> buildEvenMixBenchmarkScene(int shapeCount) {
    std::vector<Shape> shapes;
    if (shapeCount <= 0) {
        return shapes;
    }
    shapes.reserve(static_cast<std::size_t>(shapeCount));

    const float spacing = 1.35f;
    const int side =
        std::max(1, static_cast<int>(std::ceil(std::cbrt(static_cast<float>(shapeCount)))));
    const float centerOffset = 0.5f * static_cast<float>(side - 1);

    for (int i = 0; i < shapeCount; ++i) {
        Shape shape;
        shape.type = i % 7;

        const int gx = i % side;
        const int gy = (i / side) % side;
        const int gz = i / (side * side);

        shape.center[0] = (static_cast<float>(gx) - centerOffset) * spacing;
        shape.center[1] = (static_cast<float>(gy) - centerOffset) * spacing * 0.6f;
        shape.center[2] = (static_cast<float>(gz) - centerOffset) * spacing;

        shape.rotation[0] = 0.07f * static_cast<float>((i * 3) % 11);
        shape.rotation[1] = 0.05f * static_cast<float>((i * 5) % 13);
        shape.rotation[2] = 0.03f * static_cast<float>((i * 7) % 17);

        shape.scale[0] = shape.scale[1] = shape.scale[2] = 1.0f;
        shape.scaleMode = SCALE_MODE_ELONGATE;
        shape.elongation[0] = shape.elongation[1] = shape.elongation[2] = 0.0f;
        shape.fractalExtra[0] = 1.0f;
        shape.fractalExtra[1] = 1.0f;
        shape.extra = 0.0f;
        shape.twist[0] = shape.twist[1] = shape.twist[2] = 0.0f;
        shape.bend[0] = shape.bend[1] = shape.bend[2] = 0.0f;
        shape.twistModifierEnabled = false;
        shape.bendModifierEnabled = false;
        shape.mirrorModifierEnabled = false;
        shape.mirrorAxis[0] = false;
        shape.mirrorAxis[1] = false;
        shape.mirrorAxis[2] = false;
        shape.mirrorOffset[0] = 0.0f;
        shape.mirrorOffset[1] = 0.0f;
        shape.mirrorOffset[2] = 0.0f;
        shape.mirrorSmoothness = 0.0f;

        if (shape.type == SHAPE_SPHERE) {
            shape.param[0] = 0.45f;
            shape.param[1] = 0.0f;
            shape.param[2] = 0.0f;
            shape.albedo[0] = 0.90f;
            shape.albedo[1] = 0.78f;
            shape.albedo[2] = 0.75f;
        } else if (shape.type == SHAPE_BOX) {
            shape.param[0] = 0.38f;
            shape.param[1] = 0.42f;
            shape.param[2] = 0.36f;
            shape.albedo[0] = 0.72f;
            shape.albedo[1] = 0.85f;
            shape.albedo[2] = 0.95f;
        } else if (shape.type == SHAPE_TORUS) {
            shape.param[0] = 0.42f;
            shape.param[1] = 0.14f;
            shape.param[2] = 0.0f;
            shape.albedo[0] = 0.86f;
            shape.albedo[1] = 0.95f;
            shape.albedo[2] = 0.77f;
        } else if (shape.type == SHAPE_CYLINDER) {
            shape.param[0] = 0.30f;
            shape.param[1] = 0.50f;
            shape.param[2] = 0.0f;
            shape.albedo[0] = 0.83f;
            shape.albedo[1] = 0.78f;
            shape.albedo[2] = 0.96f;
        } else if (shape.type == SHAPE_CONE) {
            shape.param[0] = 0.36f;
            shape.param[1] = 0.55f;
            shape.param[2] = 0.0f;
            shape.albedo[0] = 0.74f;
            shape.albedo[1] = 0.94f;
            shape.albedo[2] = 0.84f;
        } else if (shape.type == SHAPE_MANDELBULB) {
            shape.param[0] = 8.0f;
            shape.param[1] = 10.0f;
            shape.param[2] = 4.0f;
            shape.fractalExtra[0] = 1.0f;
            shape.fractalExtra[1] = 1.0f;
            shape.albedo[0] = 0.95f;
            shape.albedo[1] = 0.62f;
            shape.albedo[2] = 0.28f;
        } else {
            shape.param[0] = 0.8f;
            shape.param[1] = 4.0f;
            shape.param[2] = 3.0f;
            shape.albedo[0] = 0.70f;
            shape.albedo[1] = 0.88f;
            shape.albedo[2] = 1.0f;
        }

        const int blendCycle = i % 3;
        if (blendCycle == 0) {
            shape.blendOp = BLEND_NONE;
        } else if (blendCycle == 1) {
            shape.blendOp = BLEND_SMOOTH_SUBTRACTION;
        } else {
            shape.blendOp = BLEND_SMOOTH_INTERSECTION;
        }

        shape.smoothness = 0.12f;
        shape.metallic = 0.0f;
        shape.roughness = 0.05f;
        shape.emission[0] = 0.0f;
        shape.emission[1] = 0.0f;
        shape.emission[2] = 0.0f;
        shape.emissionStrength = 0.0f;
        shape.transmission = 0.0f;
        shape.ior = 1.5f;
        shape.name = std::to_string(i);
        shapes.push_back(shape);
    }

    return shapes;
}

std::vector<BenchmarkSceneSpec> buildBenchmarkSceneSpecs(bool pathTracerMode) {
    std::vector<BenchmarkSceneSpec> scenes;
    scenes.reserve(3);

    BenchmarkSceneSpec simple;
    simple.id = BENCH_SCENE_SIMPLE;
    simple.key = "simple_csg";
    simple.label = "Simple Primitives + CSG";
    simple.shapes = buildSimplePrimitivesCsgBenchmarkScene();
    simple.cameraTarget[0] = 0.1f;
    simple.cameraTarget[1] = -0.15f;
    simple.cameraTarget[2] = 0.0f;
    simple.cameraDistance = 6.2f;
    simple.thetaStart = 0.2f;
    simple.thetaSpeed = 0.7f;
    simple.phiBase = 0.34f;
    simple.phiAmplitude = 0.10f;
    scenes.push_back(simple);

    BenchmarkSceneSpec repeated;
    repeated.id = BENCH_SCENE_REPEATED;
    repeated.key = "repeated_geometry";
    repeated.label = "Complex Repeated Geometry";
    repeated.shapes = buildRepeatedGeometryBenchmarkScene();
    repeated.cameraTarget[0] = 0.0f;
    repeated.cameraTarget[1] = -0.8f;
    repeated.cameraTarget[2] = 0.0f;
    repeated.cameraDistance = 10.8f;
    repeated.thetaStart = 0.35f;
    repeated.thetaSpeed = 0.55f;
    repeated.phiBase = 0.31f;
    repeated.phiAmplitude = 0.06f;
    scenes.push_back(repeated);

    BenchmarkSceneSpec worst;
    worst.id = BENCH_SCENE_WORST;
    worst.key = "thin_curvature_shadows";
    worst.label = "Worst Case Thin Features";
    if (pathTracerMode) {
        worst.shapes = buildWorstCaseThinFeatureBenchmarkScene(40, 8);
    } else {
        worst.shapes = buildWorstCaseThinFeatureBenchmarkScene();
    }
    worst.cameraTarget[0] = 0.0f;
    worst.cameraTarget[1] = -0.15f;
    worst.cameraTarget[2] = 0.0f;
    worst.cameraDistance = 8.9f;
    worst.thetaStart = -0.22f;
    worst.thetaSpeed = 0.95f;
    worst.phiBase = 0.39f;
    worst.phiAmplitude = 0.12f;
    scenes.push_back(worst);

    return scenes;
}

void sampleBenchmarkCamera(const BenchmarkSceneSpec& scene,
                           int frameIndex,
                           int totalFrames,
                           float outCameraPos[3],
                           float outCameraTarget[3]) {
    const int safeTotal = std::max(totalFrames, 1);
    const float t = static_cast<float>(frameIndex % safeTotal) / static_cast<float>(safeTotal);
    const float theta =
        scene.thetaStart + (2.0f * 3.14159265358979323846f) * scene.thetaSpeed * t;
    const float phi =
        scene.phiBase + scene.phiAmplitude * std::sin((2.0f * 3.14159265358979323846f) * t);
    outCameraTarget[0] = scene.cameraTarget[0];
    outCameraTarget[1] = scene.cameraTarget[1];
    outCameraTarget[2] = scene.cameraTarget[2];
    sphericalToCartesian(scene.cameraDistance, theta, phi, outCameraTarget, outCameraPos);
}

} // namespace rmt
