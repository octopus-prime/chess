#pragma once

#include <nnue/sqr_clipped_relu.hpp>

namespace nnue {

void test_sqr_clipped_relu_16() {
    constexpr auto N = 16;
    alignas(N) const std::int32_t input[N] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 2, 20, 200, 2000, 20000, 200000, 2000000, 20000000};
    alignas(N) std::uint8_t output[N];
    alignas(N) const std::uint8_t expected[N] = {0, 0, 0, 1, 127, 127, 127, 127, 0, 0, 0, 7, 127, 127, 127, 127};

    sqr_clipped_relu(std::span{input}, std::span{output});

    for (auto i = 0ul; i < N; ++i)
        std::printf("%d ", output[i]);
    std::printf("\n");

    if (!std::ranges::equal(output, expected))
        throw std::runtime_error{"test_sqr_clipped_relu_16 failed"};
}

}  // namespace nnue
