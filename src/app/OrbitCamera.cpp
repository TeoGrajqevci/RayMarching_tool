#include "rmt/app/OrbitCamera.h"

#include "rmt/common/math/SphericalCoordinates.h"

namespace rmt {

void updateOrbitCameraPosition(float camDistance, float camTheta, float camPhi,
                               const float cameraTarget[3], float cameraPos[3]) {
    sphericalToCartesian(camDistance, camTheta, camPhi, cameraTarget, cameraPos);
}

} // namespace rmt
