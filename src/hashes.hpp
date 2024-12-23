#pragma once

#include "common.hpp"
#include "piece.hpp"
#include "square.hpp"

struct hashes {

    static uint64_t  generate() noexcept {
        static std::mt19937_64 engine(-1ull);
        static std::uniform_int_distribution<uint64_t> distribution;
        return distribution(engine);
    }

    using lookup_t = std::array<std::array<uint64_t , SQUARE_MAX>, PIECE_MAX>;

    static uint64_t hash(piece piece, square sqaure) noexcept {
        static const lookup_t lookup = [] {
            lookup_t lookup;
            for (auto p : enum_range(WPAWN, BKING))
                for (auto s : enum_range(A1, H8))
                    lookup[p][s] = generate();
            return lookup;
        }();
        return lookup[piece][sqaure];
    }

    // static uint64_t  castle(bitboard x) noexcept {
    //     static const std::array<uint64_t , 16> lookup = [] {
    //         std::array<uint64_t , 16> lookup;
    //         std::ranges::generate(lookup, generate);
    //         return lookup;
    //     }();
    //     auto c = _pext_u64(x, "a1h1a8h8"_b);
    //     return lookup[c];
    // }

    // static uint64_t en_passant(bitboard x) noexcept {
    //     static const std::array<uint64_t, 64> lookup = [] {
    //         std::array<uint64_t, 64> lookup;
    //         std::ranges::generate(lookup, generate);
    //         return lookup;
    //     }();
    //     if (x == 0ull)
    //         return 0;
    //     return lookup[x.front()];
    // }

    static uint64_t  color(side_e side) noexcept {
        static const std::array<uint64_t , 2> lookup = [] {
            std::array<uint64_t , 2> lookup;
            std::ranges::generate(lookup, generate);
            return lookup;
        }();
        return lookup[side];
    }
};
