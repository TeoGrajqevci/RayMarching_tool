#include "rmt/common/hash/Fnv1a.h"

namespace rmt {

void fnv1aHashRaw(std::uint64_t& hash, const void* data, std::size_t sizeBytes) {
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < sizeBytes; ++i) {
        hash ^= static_cast<std::uint64_t>(bytes[i]);
        hash *= kFnv1aPrime;
    }
}

void fnv1aHashString(std::uint64_t& hash, const std::string& value) {
    const std::size_t length = value.size();
    fnv1aHashValue(hash, length);
    if (!value.empty()) {
        fnv1aHashRaw(hash, value.data(), value.size());
    }
}

} // namespace rmt
