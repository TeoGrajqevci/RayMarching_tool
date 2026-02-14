#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace rmt {

static const std::uint64_t kFnv1aOffsetBasis = 1469598103934665603ULL;
static const std::uint64_t kFnv1aPrime = 1099511628211ULL;

void fnv1aHashRaw(std::uint64_t& hash, const void* data, std::size_t sizeBytes);
void fnv1aHashString(std::uint64_t& hash, const std::string& value);

template <typename T>
inline void fnv1aHashValue(std::uint64_t& hash, const T& value) {
    fnv1aHashRaw(hash, &value, sizeof(T));
}

} // namespace rmt
