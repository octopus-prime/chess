#pragma once

#include <nnue/common.hpp>

namespace nnue {

// N = 32
template <std::size_t N>
    requires(N % sizeof(__m256i) == 0)
void clipped_relu(const std::span<const std::int32_t, N> input, const std::span<std::uint8_t, N> output) noexcept {
    std::ranges::transform(span_cast<const __m256i>(input) | std::views::chunk(4), span_cast<__m256i>(output).begin(), [](auto&& chunk) {
        const __m256i words0 = _mm256_srli_epi16(_mm256_packus_epi32(chunk[0], chunk[1]), 6);
        const __m256i words1 = _mm256_srli_epi16(_mm256_packus_epi32(chunk[2], chunk[3]), 6);
        return _mm256_permutevar8x32_epi32(_mm256_packs_epi16(words0, words1), _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0));
    });
}

// N = 16
template <std::size_t N>
    requires(N % sizeof(__m256i) != 0 && N % sizeof(__m128i) == 0)
void clipped_relu(const std::span<const std::int32_t, N> input, const std::span<std::uint8_t, N> output) noexcept {
    std::ranges::transform(span_cast<const __m128i>(input) | std::views::chunk(4), span_cast<__m128i>(output).begin(), [](auto&& chunk) {
        const __m128i words0 = _mm_srli_epi16(_mm_packus_epi32(chunk[0], chunk[1]), 6);
        const __m128i words1 = _mm_srli_epi16(_mm_packus_epi32(chunk[2], chunk[3]), 6);
        return _mm_packs_epi16(words0, words1);
    });
}

}  // namespace nnue
