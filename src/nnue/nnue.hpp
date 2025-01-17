#pragma once

#include <nnue/accumulator.hpp>
#include <nnue/features.hpp>
#include <nnue/header.hpp>
#include <nnue/index.hpp>
#include <nnue/mul_clipped_relu.hpp>
#include <nnue/network.hpp>

namespace nnue {

template <std::size_t N>
struct basic_entry {
    alignas(64) std::int16_t accumulation[N];
    std::uint64_t pieces[7];
    std::uint64_t colors[2];
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
    using Accumulator = basic_accumulator<N>;
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

    template <int Perspective>
    void refresh(Accumulator& new_accumulator, const std::span<const std::uint16_t> active_features) const noexcept {
        // features->refresh(std::span{new_accumulator.accumulation[Perspective]}, active_features);
        features->refresh(std::span{new_accumulator.accumulation[Perspective]}, std::span{new_accumulator.psqrt_accumulation[Perspective]}, active_features);
    }

    template <int Perspective>
    void update(Accumulator& new_accumulator, const Accumulator& prev_accumulator, const std::span<const std::uint16_t> removed_features, const std::span<const std::uint16_t> added_features) const noexcept {
        // features->update(std::span{new_accumulator.accumulation[Perspective]}, std::span{prev_accumulator.accumulation[Perspective]}, removed_features, added_features);
        features->update(std::span{new_accumulator.accumulation[Perspective]}, std::span{new_accumulator.psqrt_accumulation[Perspective]}, std::span{prev_accumulator.accumulation[Perspective]}, std::span{prev_accumulator.psqrt_accumulation[Perspective]}, removed_features, added_features);
    }

    // template <int Perspective>
    void update(Entry& entry, const std::span<const std::uint16_t> removed_features, const std::span<const std::uint16_t> added_features) const noexcept {
        features->update(std::span{entry.accumulation}, std::span{entry.accumulation}, removed_features, added_features);
    }

    // template <int Perspective>
    void initialize(Entry& entry) const noexcept {
        features->initialize(std::span{entry.accumulation});
    }

    template <int Perspective>
    std::int32_t evaluate(const Accumulator& accumulator, const std::size_t piece_count) const noexcept {
        const auto bucket = (piece_count - 1) / 4;
        alignas(64) std::uint8_t l1clipped[L1];

        mul_clipped_relu(std::span{accumulator.accumulation[Perspective]}, std::span{l1clipped}.template first<L1 / 2>());
        mul_clipped_relu(std::span{accumulator.accumulation[1 - Perspective]}, std::span{l1clipped}.template last<L1 / 2>());

        const auto positional = networks[bucket]->evaluate(std::span{l1clipped} | std::views::as_const);
        const auto psqt = (accumulator.psqrt_accumulation[Perspective][bucket] - accumulator.psqrt_accumulation[1 - Perspective][bucket]) / 2;

        return (positional + psqt) / 16;
        // return (125 * psqt + 131 * positional) / (128 * 16);
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
