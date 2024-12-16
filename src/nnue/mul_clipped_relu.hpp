#pragma once

#include <nnue/common.hpp>

namespace nnue {

// N = 3072 / 2 or N = 128 / 2
template <std::size_t N>
    requires(N % sizeof(__m256i) == 0)
void mul_clipped_relu(const std::span<const std::int16_t, 2 * N> input, const std::span<std::uint8_t, N> output) noexcept {
    const auto in0 = span_cast<const __m256i>(input.template first<N>());
    const auto in1 = span_cast<const __m256i>(input.template last<N>());
    std::ranges::transform(std::views::zip(in0, in1) | std::views::chunk(2), span_cast<__m256i>(output).begin(), [](auto&& chunk) {
        const __m256i sum0a = _mm256_slli_epi16(_mm256_max_epi16(_mm256_min_epi16(std::get<0>(chunk[0]), _mm256_set1_epi16(127 * 2)), _mm256_setzero_si256()), 7);
        const __m256i sum0b = _mm256_slli_epi16(_mm256_max_epi16(_mm256_min_epi16(std::get<0>(chunk[1]), _mm256_set1_epi16(127 * 2)), _mm256_setzero_si256()), 7);
        const __m256i sum1a = _mm256_min_epi16(std::get<1>(chunk[0]), _mm256_set1_epi16(127 * 2));
        const __m256i sum1b = _mm256_min_epi16(std::get<1>(chunk[1]), _mm256_set1_epi16(127 * 2));
        const __m256i pa = _mm256_mulhi_epi16(sum0a, sum1a);
        const __m256i pb = _mm256_mulhi_epi16(sum0b, sum1b);
        return _mm256_packus_epi16(pa, pb);
    });
}

}  // namespace nnue
