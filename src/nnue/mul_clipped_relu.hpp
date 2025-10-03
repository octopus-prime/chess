#pragma once

#include <nnue/common.hpp>

namespace nnue {

// N = 3072 / 2 or N = 128 / 2
template <std::size_t N>
requires(N % sizeof(__m512i) == 0)
void mul_clipped_relu(const std::span<const std::int16_t, 2 * N> input, const std::span<std::uint8_t, N> output) noexcept {
    const auto in0 = span_cast<const __m512i>(input.template first<N>());
    const auto in1 = span_cast<const __m512i>(input.template last<N>());
    std::ranges::transform(std::views::zip(in0, in1), span_cast<__m256i>(output).begin(), [](auto&& chunk) {
        const __m512i sum0 = _mm512_slli_epi16(_mm512_max_epi16(_mm512_min_epi16(std::get<0>(chunk), _mm512_set1_epi16(127 * 2)), _mm512_setzero_si512()), 7);
        const __m512i sum1 = _mm512_min_epi16(std::get<1>(chunk), _mm512_set1_epi16(127 * 2));
        const __m512i prod = _mm512_mulhi_epi16(sum0, sum1);
        return _mm256_packus_epi16(_mm512_castsi512_si256(prod), _mm512_extracti32x8_epi32(prod, 1));
    });
}

}  // namespace nnue
