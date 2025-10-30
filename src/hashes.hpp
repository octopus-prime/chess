#pragma once

#include "common.hpp"
#include "piece.hpp"
#include "square.hpp"

using hash_t = uint64_t;

struct hashes {

    static hash_t generate() noexcept {
        static std::mt19937_64 engine(-1ull);
        static std::uniform_int_distribution<hash_t> distribution;
        return distribution(engine);
    }

    using lookup_t = std::array<std::array<hash_t , SQUARE_MAX>, PIECE_MAX>;

    static hash_t hash(piece piece, square sqaure) noexcept {
        static const lookup_t lookup = [] {
            lookup_t lookup;
            for (auto p : enum_range(WPAWN, BKING))
                for (auto s : enum_range(A1, H8))
                    lookup[p][s] = generate();
            return lookup;
        }();
        return lookup[piece][sqaure];
    }

    static hash_t castle(bitboard x) noexcept {
        static const std::array<hash_t , 16> lookup = [] {
            std::array<hash_t , 16> lookup;
            std::ranges::generate(lookup, generate);
            return lookup;
        }();
        auto c = _pext_u64(x, "a1h1a8h8"_b);
        return lookup[c];
    }

    static hash_t en_passant(square x) noexcept {
        static const std::array<hash_t, 65> lookup = [] {
            std::array<hash_t, 65> lookup;
            std::ranges::generate(lookup, generate);
            lookup[NO_SQUARE] = 0;
            return lookup;
        }();
        return lookup[x];
    }

    static hash_t side() noexcept {
        static const hash_t side_hash = generate();
        return side_hash;
    };
};
