#pragma once

#include <nnue/affine_tranform.hpp>
#include <nnue/clipped_relu.hpp>
#include <nnue/sqr_clipped_relu.hpp>

namespace nnue {

template <std::size_t N>
class basic_network {
    constexpr static inline std::size_t L1 = N;
    constexpr static inline std::size_t L2 = 16;
    constexpr static inline std::size_t L3 = 32;

    alignas(64) std::int8_t weights1[L1][L2];
    alignas(64) std::int32_t biases1[L2];

    alignas(64) std::int8_t weights2[2 * L2][L3];
    alignas(64) std::int32_t biases2[L3];

    alignas(64) std::int8_t weights3[1][L3];
    alignas(64) std::int32_t biases3[1];

   public:
    basic_network(std::istream& stream) {
        std::uint32_t header;
        stream.read(reinterpret_cast<char*>(&header), sizeof(header));

        stream.read(reinterpret_cast<char*>(biases1), sizeof(biases1));
        for (auto i = 0ul; i < L1 * L2; i++) {
            auto j = get_weight_index_scrambled<L1, L2>(i);
            stream.read(reinterpret_cast<char*>(&weights1[0][0] + j), 1);
        }

        stream.read(reinterpret_cast<char*>(biases2), sizeof(biases2));
        for (auto i = 0ul; i < 2 * L2 * L3; i++) {
            auto j = get_weight_index_scrambled<2 * L2, L3>(i);
            stream.read(reinterpret_cast<char*>(&weights2[0][0] + j), 1);
        }

        stream.read(reinterpret_cast<char*>(biases3), sizeof(biases3));
        stream.read(reinterpret_cast<char*>(weights3), sizeof(weights3));
    }

    std::int32_t evaluate(const std::span<const std::uint8_t, L1> l1clipped) const noexcept {
        alignas(64) std::int32_t l2transformed[L2];
        alignas(64) std::uint8_t l2clipped[2 * L2];
        alignas(64) std::int32_t l3transformed[L3];
        alignas(64) std::uint8_t l3clipped[L3];
        alignas(64) std::int32_t l4transformed[1];

        affine_tranform<true>(std::span{l1clipped}, std::span{weights1}, std::span{biases1}, std::span{l2transformed});
        sqr_clipped_relu(std::span{l2transformed} | std::views::as_const, std::span{l2clipped}.template first<L2>());
        clipped_relu(std::span{l2transformed} | std::views::as_const, std::span{l2clipped}.template last<L2>());
        // ugly shit
        std::ranges::copy(std::span{l2clipped}.template subspan<L2, L2 - 1>(), l2clipped + L2 - 1);
        l2clipped[2 * L2 - 2] = 0;
        l2clipped[2 * L2 - 1] = 0;
        //
        affine_tranform<false>(std::span{l2clipped} | std::views::as_const, std::span{weights2}, std::span{biases2}, std::span{l3transformed});
        clipped_relu(std::span{l3transformed} | std::views::as_const, std::span{l3clipped});
        affine_tranform(std::span{l3clipped} | std::views::as_const, std::span{weights3}, std::span{biases3}, std::span{l4transformed});

        return l4transformed[0] + l2transformed[L2 - 1] * (600 * 16) / (127 * (1 << 6));
    }
};

}  // namespace nnue
