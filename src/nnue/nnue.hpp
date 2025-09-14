#pragma once

#include <nnue/features.hpp>
#include <nnue/header.hpp>
#include <nnue/index.hpp>
#include <nnue/mul_clipped_relu.hpp>
#include <nnue/network.hpp>

namespace nnue {

template <std::size_t N>
struct basic_entry {
    alignas(64) std::int16_t accumulation[N]{};
    std::int32_t psqrt_accumulation[8]{};
    std::uint64_t pieces[7]{};
    std::uint64_t colors[2]{};
};

template <std::size_t N>
class basic_nnue {
    using Header = header;
    using Features = basic_features<N>;
    using Network = basic_network<N>;

    std::unique_ptr<Header> header;
    std::unique_ptr<Features> features;
    std::unique_ptr<Network> networks[8];

   public:
    using Entry = basic_entry<N>;

    constexpr static inline std::size_t L1 = N;

    basic_nnue();

    basic_nnue(const std::string_view filename) {
        std::ifstream stream{filename.data(), std::ios::binary};

        header = std::make_unique<Header>(stream);
        features = std::make_unique<Features>(stream);
        std::ranges::generate(networks, [&]() {
            return std::make_unique<Network>(stream);
        });

        if (!stream || stream.fail() || stream.peek() != std::ios::traits_type::eof())
            throw std::runtime_error("failed to read network");
    }

    std::uint32_t version() const noexcept {
        return header->version;
    }

    std::uint32_t hash() const noexcept {
        return header->hash;
    }

    std::string_view description() const noexcept {
        return header->description;
    }

    void update(Entry& entry, const std::span<const std::uint16_t> removed_features, const std::span<const std::uint16_t> added_features) const noexcept {
        features->update(std::span{entry.accumulation}, std::span{entry.psqrt_accumulation}, removed_features, added_features);
    }

    void initialize(Entry& entry) const noexcept {
        features->initialize(std::span{entry.accumulation}, std::span{entry.psqrt_accumulation});
    }

    std::int32_t evaluate(const Entry& t, const Entry& o, const std::size_t piece_count) const noexcept {
        const auto bucket = (piece_count - 1) / 4;
        alignas(64) std::uint8_t l1clipped[L1];


        mul_clipped_relu(std::span{t.accumulation}, std::span{l1clipped}.template first<L1 / 2>());
        mul_clipped_relu(std::span{o.accumulation}, std::span{l1clipped}.template last<L1 / 2>());

        const auto positional = networks[bucket]->evaluate(std::span{l1clipped} | std::views::as_const);
        const auto psqt = (t.psqrt_accumulation[bucket] - o.psqrt_accumulation[bucket]) / 2;

        return (positional + psqt) / 64;
    }
};

template <>
basic_nnue<128>::basic_nnue() : basic_nnue{small_nnue_filename} {
}

template <>
basic_nnue<3072>::basic_nnue() : basic_nnue{big_nnue_filename} {
}

using small_nnue = basic_nnue<128>;
using big_nnue = basic_nnue<3072>;

}  // namespace nnue
