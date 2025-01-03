#pragma once

#include <nnue/common.hpp>

namespace nnue {

template <std::size_t N>
struct basic_accumulator {
    alignas(64)  std::int16_t accumulation[2][N];
    std::int32_t psqrt_accumulation[2][8];
    bool         computed[2] = {false, false};
};

}  // namespace nnue
