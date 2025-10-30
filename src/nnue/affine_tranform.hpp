#pragma once

#include <nnue/common.hpp>

namespace nnue {

template <std::size_t I, std::size_t O>
auto get_weight_index_scrambled(std::size_t i) noexcept {
    return (i / 4) % (I / 4) * O * 4 + i / I * 4 + i % 4;
}

// template <std::size_t I>
// std::span<std::uint16_t> find_nnz(const std::span<const std::int32_t, I> input, const std::span<std::uint16_t, I> out) noexcept {
//     const __m512i       increment    = _mm512_set1_epi16(32);
//     __m512i             base = _mm512_set_epi16(  // Same permute order as _mm512_packus_epi32()
//       31, 30, 29, 28, 15, 14, 13, 12, 27, 26, 25, 24, 11, 10, 9, 8, 23, 22, 21, 20, 7, 6, 5, 4, 19, 18, 17, 16, 3, 2, 1, 0);
//     auto count = 0;
//     for (auto&& chunk : span_cast<const __m512i>(input) | std::views::chunk(2)) {
//         const __m512i   inputV01 = _mm512_packus_epi32(chunk[0], chunk[1]);
//         const __mmask32 nnzMask  = _mm512_test_epi16_mask(inputV01, inputV01);
//         _mm512_mask_compressstoreu_epi16(out.data() + count, nnzMask, base);
//         count += std::popcount(nnzMask);
//         base = _mm512_add_epi16(base, increment);
//     }
//     return out.first(count);
// }

// // column major
// template <bool sparse, std::size_t I, std::size_t O>
//     requires(I % 4 == 0 && O % (sizeof(__m512i) / sizeof(std::int32_t)) == 0)
// void affine_tranform(const std::span<const std::uint8_t, I> input, const std::span<const std::int8_t[O], I> weights, const std::span<const std::int32_t, O> biases, const std::span<std::int32_t, O> output) noexcept {
//     constexpr auto OutputSimdWidth = sizeof(__m512i) / sizeof(std::int32_t);
//     constexpr auto NumChunks = I / 4;
//     constexpr auto NumRegs = O / OutputSimdWidth;

//     const auto input32 = span_cast<const std::int32_t>(input);
//     const auto biasvec = span_cast<const __m512i>(biases);
//     const auto outptr = span_cast<__m512i>(output);

//     __m512i acc[NumRegs];
//     const auto f = [&](const std::uint16_t i) -> void {
//         const auto in0 = _mm512_set1_epi32(input32[i]);
//         const auto col0 = span_cast<const __m512i>(std::span{weights[i * 4]});
//         for (auto k = 0ul; k < NumRegs; ++k)
//             acc[k] = _mm512_dpbusd_epi32(acc[k], in0, col0[k]);
//     };

//     if constexpr (sparse) {
//         alignas(64) std::uint16_t buf[NumChunks];
//         auto nnz = find_nnz(input32, std::span{buf});
//         std::ranges::copy(biasvec, acc);
//         std::ranges::for_each(nnz, f);
//         std::ranges::copy(acc, outptr.begin());
//     } else {
//         std::ranges::copy(biasvec, acc);
//         std::ranges::for_each(std::views::iota(0ul, NumChunks), f);
//         std::ranges::copy(acc, outptr.begin());
//     }

//     // for (auto i = 0ul; i < O; ++i)
//     //     output[i] = biases[i];
//     // for (auto i = 0ul; i < I; ++i)
//     //     // if (input[i]) {
//     //         for (auto j = 0ul; j < O; ++j)
//     //             output[j] += weights[j][i] * input[i];
//     //     // }
// }

// void affine_tranform(const std::span<const std::uint8_t, 32> input, const std::span<const std::int8_t[32], 32> weights, const std::span<const std::int32_t, 32> biases, const std::span<std::int32_t, 32> output) noexcept {
//     // column major
//     const auto input32 = span_cast<const std::int32_t>(input); // 8
//     const auto biasvec = span_cast<const __m512i>(biases);  // 2
//     const auto outptr = span_cast<__m512i>(output); // 2
//     __m512i acc0 = biasvec[0];
//     __m512i acc1 = biasvec[1];
//     for (auto i = 0ul; i < input32.size(); ++i) {
//         const auto in = _mm512_set1_epi32(input32[i]);
//         const auto col = span_cast<const __m512i>(std::span{weights[i * 4]});
//         acc0 = _mm512_dpbusd_epi32(acc0, in, col[0]);
//         acc1 = _mm512_dpbusd_epi32(acc1, in, col[1]);
//     }
//     outptr[0] = acc0;
//     outptr[1] = acc1;
// }

void affine_tranform(const std::span<const std::uint8_t, 3072> input, const std::span<const std::int8_t[16], 3072> weights, const std::span<const std::int32_t, 16> biases, const std::span<std::int32_t, 16> output) noexcept {
    // column major
    const auto input32 = span_cast<const std::int32_t>(input); // 768
    alignas(64) std::uint16_t buf[input32.size()];
    // auto nnz = find_nnz(input32, std::span{buf});

    const __m512i       increment    = _mm512_set1_epi16(32);
    __m512i base = _mm512_set_epi16(31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
                                    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    auto count = 0;
    for (auto&& chunk : span_cast<const __m512i>(input32) | std::views::chunk(2)) {
        const __mmask16 mask0 = _mm512_test_epi32_mask(chunk[0], chunk[0]);
        const __mmask16 mask1 = _mm512_test_epi32_mask(chunk[1], chunk[1]);
        const __mmask32 nnzMask = _mm512_kunpackw(mask1, mask0);
        _mm512_mask_compressstoreu_epi16(buf + count, nnzMask, base);
        count += std::popcount(nnzMask);
        base = _mm512_add_epi16(base, increment);
    }

    __m512i acc = _mm512_load_si512(biases.data());
    for (auto i : std::span{buf}.first(count)) {
        const __m512i in = _mm512_set1_epi32(input32[i]);
        acc = _mm512_dpbusd_epi32(acc, in, _mm512_load_si512(weights[i * 4]));
    }
    _mm512_store_si512(output.data(), acc);
}

void affine_tranform(const std::span<const std::uint8_t, 128> input, const std::span<const std::int8_t[16], 128> weights, const std::span<const std::int32_t, 16> biases, const std::span<std::int32_t, 16> output) noexcept {
    // column major
    const auto input32 = span_cast<const std::int32_t>(input); // 32
    __m512i acc = _mm512_load_si512(biases.data());
    for (auto i = 0ul; i < input32.size(); ++i) {
        const __m512i in = _mm512_set1_epi32(input32[i]);
        acc = _mm512_dpbusd_epi32(acc, in, _mm512_load_si512(weights[i * 4]));
    }
    _mm512_store_si512(output.data(), acc);
}

void affine_tranform(const std::span<const std::uint8_t, 32> input, const std::span<const std::int8_t[32], 32> weights, const std::span<const std::int32_t, 32> biases, const std::span<std::int32_t, 32> output) noexcept {
    // column major
    const auto input32 = span_cast<const std::int32_t>(input); // 8
    __m512i acc0 = _mm512_load_si512(biases.first<16>().data());
    __m512i acc1 = _mm512_load_si512(biases.last<16>().data());
    for (auto i = 0ul; i < input32.size(); ++i) {
        const __m512i in = _mm512_set1_epi32(input32[i]);
        acc0 = _mm512_dpbusd_epi32(acc0, in, _mm512_load_si512(weights[i * 4 + 0]));
        acc1 = _mm512_dpbusd_epi32(acc1, in, _mm512_load_si512(weights[i * 4 + 2]));
    }
    _mm512_store_si512(output.first<16>().data(), acc0);
    _mm512_store_si512(output.last<16>().data(), acc1);
}

void affine_tranform(const std::span<const std::uint8_t, 32> input, const std::span<const std::int8_t[32], 1> weights, const std::span<const std::int32_t, 1> biases, const std::span<std::int32_t, 1> output) noexcept {
    // row major
    const __m256i in = _mm256_load_si256(reinterpret_cast<const __m256i*>(input.data()));
    const __m256i row = _mm256_load_si256(reinterpret_cast<const __m256i*>(weights.data()));
    const __m256i sum = _mm256_madd_epi16(_mm256_maddubs_epi16(in, row), _mm256_set1_epi16(1));
    output[0] = biases[0] + __builtin_reduce_add((__v8si) sum);
}

}  // namespace nnue
