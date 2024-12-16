#pragma once

#include <nnue/clipped_relu.hpp>

namespace nnue {

void test_clipped_relu_16() {
    constexpr auto N = 16;
    alignas(64) const std::int32_t input[N] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 2, 20, 200, 2000, 20000, 200000, 2000000, 20000000};
    alignas(64) std::uint8_t output[N];
    alignas(64) const std::uint8_t expected[N] = {0, 0, 1, 15, 127, 127, 127, 127, 0, 0, 3, 31, 127, 127, 127, 127};

    clipped_relu(std::span{input}, std::span{output});

    for (auto i = 0ul; i < N; ++i)
        std::printf("%d ", output[i]);
    std::printf("\n");

    if (!std::ranges::equal(output, expected))
        throw std::runtime_error{"test_clipped_relu_16 failed"};
}

void test_clipped_relu_32() {
    constexpr auto N = 32;
    alignas(64) const std::int32_t input[N] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 2, 20, 200, 2000, 20000, 200000, 2000000, 20000000, 3, 30, 300, 3000, 30000, 300000, 3000000, 30000000, 5, 50, 500, 5000, 50000, 500000, 5000000, 50000000};
    alignas(64) std::uint8_t output[N];
    alignas(64) const std::uint8_t expected[N] = {0, 0, 1, 15, 127, 127, 127, 127, 0, 0, 3, 31, 127, 127, 127, 127, 0, 0, 4, 46, 127, 127, 127, 127, 0, 0, 7, 78, 127, 127, 127, 127};

    clipped_relu(std::span{input}, std::span{output});

    for (auto i = 0ul; i < N; ++i)
        std::printf("%d ", output[i]);
    std::printf("\n");

    if (!std::ranges::equal(output, expected))
        throw std::runtime_error{"test_clipped_relu_32 failed"};
}

}  // namespace nnue
