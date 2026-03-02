#pragma once

namespace rmt {

float absSum3(const float v[3]);
bool anyAbsGreaterThan(const float v[3], float threshold);
bool nonUnitScale(const float v[3], float threshold);

void setIdentity3(float out[9]);
void buildInverseRotationRows(const float angles[3], float out[9]);
void applyInverseRotationRows(const float rows[9], const float p[3], float out[3]);

void applyRotationInverse(const float p[3], const float angles[3], float out[3]);
void applyTwistCombinedCPU(const float p[3], const float k[3], float out[3]);
void applyCheapBendCombinedCPU(const float p[3], const float k[3], float out[3]);
void applyDomainRepeatCombinedCPU(const float p[3], const float axisEnabled[3], const float spacing[3], float out[3]);

} // namespace rmt
