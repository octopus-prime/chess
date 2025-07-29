#pragma once

#include <nnue/common.hpp>
#include <nnue/accumulator.hpp>

namespace nnue {

// Read N signed integers from the stream s, putting them in the array out.
// The stream is assumed to be compressed using the signed LEB128 format.
// See https://en.wikipedia.org/wiki/LEB128 for a description of the compression scheme.

template<typename T, std::size_t N>
    requires std::is_signed_v<T>
void read_leb_128(std::istream& stream, const std::span<T, N> buffer) {
    constexpr std::string_view COMPRESSED_LEB128 = "COMPRESSED_LEB128"sv;

    // Check the presence of our LEB128 magic string
    char leb128MagicString[COMPRESSED_LEB128.size()];
    stream.read(leb128MagicString, COMPRESSED_LEB128.size());

    if (COMPRESSED_LEB128 != std::string_view{leb128MagicString, COMPRESSED_LEB128.size()})
        throw std::runtime_error("COMPRESSED_LEB128 not matched");

    std::uint32_t bytes_left;
    stream.read(reinterpret_cast<char*>(&bytes_left), sizeof(bytes_left));

    std::array<std::uint8_t, 4096> buf;
    std::uint32_t buf_pos = buf.size();
    for (std::size_t i = 0; i < buffer.size(); ++i) {
        T result = 0;
        std::size_t shift = 0;
        do {
            if (buf_pos == buf.size()) {
                stream.read(reinterpret_cast<char*>(buf.data()), std::min<std::uint32_t>(bytes_left, buf.size()));
                buf_pos = 0;
            }

            std::uint8_t byte = buf[buf_pos++];
            result |= (byte & 0x7f) << shift;
            --bytes_left;
            shift += 7;

            if ((byte & 0x80) == 0) {
                buffer[i] = (sizeof(T) * 8 <= shift || (byte & 0x40) == 0)
                         ? result
                         : result | ~((1 << shift) - 1);
                break;
            }
        } while (shift < sizeof(T) * 8);
    }
    // assert(bytes_left == 0);
}

template <std::size_t N>
class basic_features {
    using Accumulator = basic_accumulator<N>;

    constexpr static inline std::size_t L0 = 22528;
    constexpr static inline std::size_t L1 = N;

    alignas(64) std::int16_t weights0[L0][L1];
    alignas(64) std::int16_t biases0[L1];
    alignas(64) std::int32_t psqrt_weights0[L0][8];

public:
    basic_features(std::istream& stream) {
        const auto biases = std::span{biases0};
        const auto weights = std::span<std::int16_t, L1 * L0>{&weights0[0][0], L1 * L0};
        const auto psqrt_weights = std::span<std::int32_t, 8 * L0>{&psqrt_weights0[0][0], 8 * L0};
        std::uint32_t header;

        // read
        stream.read(reinterpret_cast<char*>(&header), sizeof(header));
        read_leb_128(stream, biases);
        read_leb_128(stream, weights);
        read_leb_128(stream, psqrt_weights);

        // permute
        for (auto&& chunk : span_cast<std::uint64_t>(biases) | std::views::chunk(8)) {
            std::swap(chunk[2], chunk[4]);
            std::swap(chunk[3], chunk[5]);
        }
        for (auto&& chunk : span_cast<std::uint64_t>(weights) | std::views::chunk(8)) {
            std::swap(chunk[2], chunk[4]);
            std::swap(chunk[3], chunk[5]);
        }

        // scale
        for (auto&& value : biases)
            value *= 2;
        for (auto&& value : weights)
            value *= 2;
    }

    void refresh(const std::span<std::int16_t, N> new_accumulation, const std::span<std::int32_t, 8> new__psqrt_accumulation, const std::span<const std::uint16_t> active_features) const noexcept {
        constexpr auto chunk = std::min(std::size_t{16}, N * sizeof(std::int16_t) / sizeof(__m256i)); // 16 num simd regs (N=3072) - or - 8 just all data (N=128)
        static_assert(N % (chunk * sizeof(__m256i) / sizeof(std::int16_t)) == 0);
        const auto accumulation = span_cast<__m256i>(new_accumulation);
        const auto biases = span_cast<const __m256i>(std::span{biases0});
        const auto weights = [this](const auto feature){ return span_cast<const __m256i>(std::span{weights0[feature]}); };
        for (auto index = 0ul; index < accumulation.size(); index += chunk) {
            __m256i regs[chunk];
            std::ranges::copy(biases.subspan(index, chunk), regs);
            for (auto feature : active_features)
                std::ranges::transform(regs, weights(feature).subspan(index, chunk), regs, _mm256_add_epi16);
            std::ranges::copy(regs, accumulation.subspan(index, chunk).begin());
        }

        const auto psqrt_accumulation = span_cast<__m256i>(new__psqrt_accumulation);
        const auto psqrt_weights = [this](const auto feature){ return span_cast<const __m256i>(std::span{psqrt_weights0[feature]}); };
        std::ranges::fill(psqrt_accumulation, _mm256_setzero_si256());
        for (auto feature : active_features)
            std::ranges::transform(psqrt_accumulation, psqrt_weights(feature), psqrt_accumulation.begin(), _mm256_add_epi32);
    }

    void update(const std::span<std::int16_t, N> new_accumulation, const std::span<std::int32_t, 8> new__psqrt_accumulation, const std::span<const std::int16_t, N> prev_accumulation, const std::span<const std::int32_t, 8> prev__psqrt_accumulation, const std::span<const std::uint16_t> removed_features, const std::span<const std::uint16_t> added_features) const noexcept {
        constexpr auto chunk = std::min(std::size_t{16}, N * sizeof(std::int16_t) / sizeof(__m256i)); // 16 num simd regs (N=3072) - or - 8 just all data (N=128)
        static_assert(N % (chunk * sizeof(__m256i) / sizeof(std::int16_t)) == 0);
        const auto accumulation = span_cast<__m256i>(new_accumulation);
        const auto previous = span_cast<const __m256i>(prev_accumulation);
        const auto weights = [this](const auto feature){ return span_cast<const __m256i>(std::span{weights0[feature]}); };
        for (auto index = 0ul; index < accumulation.size(); index += chunk) {
            __m256i regs[chunk];
            std::ranges::copy(previous.subspan(index, chunk), regs);
            for (auto feature : removed_features)
                std::ranges::transform(regs, weights(feature).subspan(index, chunk), regs, _mm256_sub_epi16);
            for (auto feature : added_features)
                std::ranges::transform(regs, weights(feature).subspan(index, chunk), regs, _mm256_add_epi16);
            std::ranges::copy(regs, accumulation.subspan(index, chunk).begin());
        }

        const auto psqrt_accumulation = span_cast<__m256i>(new__psqrt_accumulation);
        const auto previous_psqrt = span_cast<const __m256i>(prev__psqrt_accumulation);
        const auto psqrt_weights = [this](const auto feature){ return span_cast<const __m256i>(std::span{psqrt_weights0[feature]}); };
        std::ranges::copy(previous_psqrt, psqrt_accumulation.begin());
        for (auto feature : removed_features)
            std::ranges::transform(psqrt_accumulation, psqrt_weights(feature), psqrt_accumulation.begin(), _mm256_sub_epi32);
        for (auto feature : added_features)
            std::ranges::transform(psqrt_accumulation, psqrt_weights(feature), psqrt_accumulation.begin(), _mm256_add_epi32);
    }

    void initialize(const std::span<std::int16_t, N> new_accumulation, const std::span<std::int32_t, 8> new__psqrt_accumulation) const noexcept {
        const auto accumulation = span_cast<__m256i>(new_accumulation);
        const auto biases = span_cast<const __m256i>(std::span{biases0});
        std::ranges::copy(biases, accumulation.begin());

        const auto psqrt_accumulation = span_cast<__m256i>(new__psqrt_accumulation);
        std::ranges::fill(psqrt_accumulation, _mm256_setzero_si256());
    }
};

}  // namespace nnue
