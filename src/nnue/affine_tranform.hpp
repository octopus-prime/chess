#pragma once

#include <nnue/common.hpp>

namespace nnue {

template <std::size_t I, std::size_t O>
auto get_weight_index_scrambled(std::size_t i) noexcept {
    return (i / 4) % (I / 4) * O * 4 + i / I * 4 + i % 4;
}

template <std::size_t I, std::size_t O>
std::span<std::uint16_t> find_nnz(const std::span<const std::int32_t, I> input, const std::span<std::uint16_t, O> out) noexcept {
    const __m512i       increment    = _mm512_set1_epi16(32);
    __m512i             base = _mm512_set_epi16(  // Same permute order as _mm512_packus_epi32()
      31, 30, 29, 28, 15, 14, 13, 12, 27, 26, 25, 24, 11, 10, 9, 8, 23, 22, 21, 20, 7, 6, 5, 4, 19, 18, 17, 16, 3, 2, 1, 0);
    auto count = 0;
    for (auto&& chunk : span_cast<const __m512i>(input) | std::views::chunk(2)) {
        const __m512i   inputV01 = _mm512_packus_epi32(chunk[0], chunk[1]);
        const __mmask32 nnzMask  = _mm512_test_epi16_mask(inputV01, inputV01);
        _mm512_mask_compressstoreu_epi16(out.data() + count, nnzMask, base);
        count += std::popcount(nnzMask);
        base = _mm512_add_epi16(base, increment);
    }
    return out.first(count);
}

// column major
template <bool sparse, std::size_t I, std::size_t O>
    requires(I % 4 == 0 && O % (sizeof(__m512i) / sizeof(std::int32_t)) == 0)
void affine_tranform(const std::span<const std::uint8_t, I> input, const std::span<const std::int8_t[O], I> weights, const std::span<const std::int32_t, O> biases, const std::span<std::int32_t, O> output) noexcept {
    constexpr auto OutputSimdWidth = sizeof(__m512i) / sizeof(std::int32_t);
    constexpr auto NumChunks = I / 4;
    constexpr auto NumRegs = O / OutputSimdWidth;

    const auto input32 = span_cast<const std::int32_t>(input);
    const auto biasvec = span_cast<const __m512i>(biases);
    const auto outptr = span_cast<__m512i>(output);

    __m512i acc[NumRegs];
    const auto f = [&](const std::uint16_t i) -> void {
        const auto in0 = _mm512_set1_epi32(input32[i]);
        const auto col0 = span_cast<const __m512i>(std::span{weights[i * 4]});
        for (auto k = 0ul; k < NumRegs; ++k)
            acc[k] = _mm512_dpbusd_epi32(acc[k], in0, col0[k]);
    };

    if constexpr (sparse) {
        alignas(64) std::uint16_t buf[NumChunks];
        auto nnz = find_nnz(input32, std::span{buf});
        std::ranges::copy(biasvec, acc);
        std::ranges::for_each(nnz, f);
        std::ranges::copy(acc, outptr.begin());
    } else {
        std::ranges::copy(biasvec, acc);
        std::ranges::for_each(std::views::iota(0ul, NumChunks), f);
        std::ranges::copy(acc, outptr.begin());
    }

    // for (auto i = 0ul; i < O; ++i)
    //     output[i] = biases[i];
    // for (auto i = 0ul; i < I; ++i)
    //     // if (input[i]) {
    //         for (auto j = 0ul; j < O; ++j)
    //             output[j] += weights[j][i] * input[i];
    //     // }
}

// row major
template <std::size_t I, std::size_t O>
    requires(I % sizeof(__m256i) == 0)
void affine_tranform(const std::span<const std::uint8_t, I> input, const std::span<const std::int8_t[I], O> weights, const std::span<const std::int32_t, O> biases, const std::span<std::int32_t, O> output) noexcept {
    // constexpr auto hadd = [](const __m256i sum) -> std::int32_t {
    //     __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
    //     sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_PERM_BADC));
    //     sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_PERM_CDAB));
    //     return _mm_cvtsi128_si32(sum128);
    // };

    const auto in = span_cast<const __m256i>(input);

    for (auto i = 0ul; i < O; ++i) {
        const auto row = span_cast<const __m256i>(std::span{weights[i]});

        const __m256i sum = std::ranges::fold_left(std::views::zip(in, row), _mm256_setzero_si256(), [](const __m256i acc, auto&& zip) -> __m256i {
            return _mm256_add_epi32(acc, _mm256_madd_epi16(_mm256_maddubs_epi16(std::get<0>(zip), std::get<1>(zip)), _mm256_set1_epi16(1)));
        });

        output[i] = biases[i] + __builtin_reduce_add((__v8si) sum);
    }

    // for (auto i = 0ul; i < O; ++i)
    //     output[i] = biases[i];
    // for (auto i = 0ul; i < I; ++i)
    //     for (auto j = 0ul; j < O; ++j)
    //         output[j] += weights[j][i] * input[i];
}

}  // namespace nnue
