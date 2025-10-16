#pragma once

#include <nnue/nnue.hpp>
#include "../ut.hpp"

namespace nnue {

void test_nnue() {
    using NNUE = big_nnue;

    namespace ut = boost::ut;
    using ut::operator""_test;

    "test_nnue"_test = [] {
        const NNUE nnue;

        const auto version = nnue.version();
        const auto hash = nnue.hash();
        const auto description = nnue.description();

        ut::expect(ut::eq(version, 2062757664));
        ut::expect(ut::eq(hash, 470819058));
        ut::expect(ut::eq(description, "26bgL66J1BIzGUd with the https://github.com/official-stockfish/nnue-pytorch trainer."sv));

        NNUE::Entry entry[2];
        nnue.initialize(entry[WHITE]);
        nnue.initialize(entry[BLACK]);

        // evaluation with refresh

        const std::uint16_t active_features[2][32] = { {
                make_index(A1, A1, WKING, WHITE),
                make_index(A1, B8, BKING, WHITE),
                make_index(A1, C3, WPAWN, WHITE),
                make_index(A1, D4, BROOK, WHITE)
            }, {
                make_index(B8, A1, WKING, BLACK),
                make_index(B8, B8, BKING, BLACK),
                make_index(B8, C3, WPAWN, BLACK),
                make_index(B8, D4, BROOK, BLACK)
            }
        };

        nnue.update(entry[WHITE], {}, std::span{active_features[WHITE]}.first(4));
        nnue.update(entry[BLACK], {}, std::span{active_features[BLACK]}.first(4));

        ut::expect(ut::eq(nnue.evaluate(entry[WHITE], entry[BLACK], 4), -420));
        ut::expect(ut::eq(nnue.evaluate(entry[BLACK], entry[WHITE], 4), 562));

        // evaluation with update

        const std::uint16_t removed_features[2][3] = { {
                make_index(A1, C3, WPAWN, WHITE),
                make_index(A1, D4, BROOK, WHITE)
            }, {
                make_index(B8, C3, WPAWN, BLACK),
                make_index(B8, D4, BROOK, BLACK)
        }};

        const std::uint16_t added_features[2][3] = { {
                make_index(A1, D4, WPAWN, WHITE)
            }, {
                make_index(B8, D4, WPAWN, BLACK)
        }};

        nnue.update(entry[WHITE], std::span{removed_features[WHITE]}.first(2), std::span{added_features[WHITE]}.first(1));
        nnue.update(entry[BLACK], std::span{removed_features[BLACK]}.first(2), std::span{added_features[BLACK]}.first(1));

        ut::expect(ut::eq(nnue.evaluate(entry[WHITE], entry[BLACK], 3), 8));
        ut::expect(ut::eq(nnue.evaluate(entry[BLACK], entry[WHITE], 3), 23));
    };
}

}  // namespace nnue
