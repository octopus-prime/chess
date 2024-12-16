#pragma once

#include <nnue/common.hpp>

namespace nnue {

struct header {
    std::uint32_t version;
    std::uint32_t hash;
    std::string description;

    header(std::istream& stream) {
        std::uint32_t size;
        stream.read(reinterpret_cast<char*>(&version), sizeof(version));
        stream.read(reinterpret_cast<char*>(&hash), sizeof(hash));
        stream.read(reinterpret_cast<char*>(&size), sizeof(size));
        description.resize(size);
        stream.read(description.data(), size);
    }
};

}  // namespace nnue
