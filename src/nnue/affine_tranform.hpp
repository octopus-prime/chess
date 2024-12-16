#pragma once

#include <nnue/common.hpp>

namespace nnue {

alignas(64) constexpr std::array<std::array<std::uint16_t, 8>, 256> lookup_indices = []() {
    std::array<std::array<std::uint16_t, 8>, 256> v{};
    for (unsigned i = 0; i < 256; ++i) {
        std::uint64_t j = i, k = 0;
        while (j) {
            auto n = std::countr_zero(j);
            v[i][k++] = n;
            j ^= 1ull << n;
        }
    }
    return v;
}();

template <std::size_t I, std::size_t O>
auto get_weight_index_scrambled(std::size_t i) noexcept {
    return (i / 4) % (I / 4) * O * 4 + i / I * 4 + i % 4;
}

template <std::size_t I, std::size_t O>
auto find_nnz(const std::span<const std::int32_t, I> input, const std::span<std::uint16_t, O> out) noexcept {
    constexpr auto vec_nnz = [](const __m256i x) -> std::uint32_t {
        return _mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpgt_epi32(x, _mm256_setzero_si256())));
    };

    constexpr auto InputSimdWidth = sizeof(__m256i) / sizeof(std::int32_t);
    constexpr auto ChunkSize = std::max(InputSimdWidth, 8ul);
    constexpr auto NumChunks = I / ChunkSize;
    constexpr auto InputsPerChunk = ChunkSize / InputSimdWidth;
    constexpr auto OutputsPerChunk = ChunkSize / 8;

    const auto inputVector = span_cast<const __m256i>(input);
    const auto increment = _mm_set1_epi16(8);
    auto base = _mm_setzero_si128();
    auto count = 0ul;
    for (auto i = 0ul; i < NumChunks; ++i) {
        std::uint32_t nnz = 0ul;
        for (auto j = 0ul; j < InputsPerChunk; ++j) {
            const __m256i inputChunk = inputVector[i * InputsPerChunk + j];
            nnz |= vec_nnz(inputChunk) << (j * InputSimdWidth);
        }
        for (auto j = 0ul; j < OutputsPerChunk; ++j) {
            const auto lookup = (nnz >> (j * 8)) & 0xFF;
            const auto offsets = *reinterpret_cast<const __m128i*>(&lookup_indices[lookup]);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&out[count]), _mm_add_epi16(base, offsets));
            // *reinterpret_cast<__m128i*>(&out[count]) = _mm_add_epi16(base, offsets);
            count += std::popcount(lookup);
            base = _mm_add_epi16(base, increment);
        }
    }
    return out.first(count);
}

// column major
template <bool sparse, std::size_t I, std::size_t O>
    requires(I % 4 == 0 && O % (sizeof(__m256i) / sizeof(std::int32_t)) == 0)
void affine_tranform(const std::span<const std::uint8_t, I> input, const std::span<const std::int8_t[O], I> weights, const std::span<const std::int32_t, O> biases, const std::span<std::int32_t, O> output) noexcept {
    constexpr auto vec_add_dpbusd_32 = [](const __m256i acc, const __m256i x, const __m256i y) -> __m256i {
        return _mm256_add_epi32(acc, _mm256_madd_epi16(_mm256_maddubs_epi16(x, y), _mm256_set1_epi16(1)));
    };

    constexpr auto OutputSimdWidth = sizeof(__m256i) / sizeof(std::int32_t);
    constexpr auto NumChunks = I / 4;
    constexpr auto NumRegs = O / OutputSimdWidth;

    const auto input32 = span_cast<const std::int32_t>(input);
    const auto biasvec = span_cast<const __m256i>(biases);
    const auto outptr = span_cast<__m256i>(output);

    __m256i acc[NumRegs];
    const auto f = [&](const std::uint16_t i) -> void {
        const auto in0 = _mm256_set1_epi32(input32[i]);
        const auto col0 = span_cast<const __m256i>(std::span{weights[i * 4]});
        for (auto k = 0ul; k < NumRegs; ++k)
            acc[k] = vec_add_dpbusd_32(acc[k], in0, col0[k]);
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
    constexpr auto hadd = [](const __m256i sum) -> std::int32_t {
        __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
        sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_PERM_BADC));
        sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_PERM_CDAB));
        return _mm_cvtsi128_si32(sum128);
    };

    const auto in = span_cast<const __m256i>(input);

    for (auto i = 0ul; i < O; ++i) {
        const auto row = span_cast<const __m256i>(std::span{weights[i]});

        const __m256i sum = std::ranges::fold_left(std::views::zip(in, row), _mm256_setzero_si256(), [](const __m256i acc, auto&& zip) -> __m256i {
            return _mm256_add_epi32(acc, _mm256_madd_epi16(_mm256_maddubs_epi16(std::get<0>(zip), std::get<1>(zip)), _mm256_set1_epi16(1)));
        });

        output[i] = biases[i] + hadd(sum);
    }

    // for (auto i = 0ul; i < O; ++i)
    //     output[i] = biases[i];
    // for (auto i = 0ul; i < I; ++i)
    //     for (auto j = 0ul; j < O; ++j)
    //         output[j] += weights[j][i] * input[i];
}

}  // namespace nnue
