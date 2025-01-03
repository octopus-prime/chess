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

        // std::printf("%d\n", version);
        // std::printf("%d\n", hash);
        // std::printf("%s\n", description.data());

        ut::expect(version == 2062757664);
        ut::expect(hash == 470819058);
        ut::expect(description == "u0pi21KGe9rz0Ed with the https://github.com/official-stockfish/nnue-pytorch trainer."sv);

        NNUE::Accumulator accumulator[2];
        std::int32_t score[2];

        // evaluation with refresh

        const std::uint16_t active_features[2][32] = { {
                make_index<WHITE>(A1, A1, WKING),
                make_index<WHITE>(A1, B8, BKING),
                make_index<WHITE>(A1, C3, WPAWN),
                make_index<WHITE>(A1, D4, BROOK)
            }, {
                make_index<BLACK>(B8, A1, WKING),
                make_index<BLACK>(B8, B8, BKING),
                make_index<BLACK>(B8, C3, WPAWN),
                make_index<BLACK>(B8, D4, BROOK)
            }
        };

        nnue.refresh<WHITE>(accumulator[0], std::span{active_features[WHITE]}.first(4));
        nnue.refresh<BLACK>(accumulator[0], std::span{active_features[BLACK]}.first(4));

        score[WHITE] = nnue.evaluate<WHITE>(accumulator[0], 4);
        score[BLACK] = nnue.evaluate<BLACK>(accumulator[0], 4);

        // std::printf("%d\n", score[WHITE]);
        // std::printf("%d\n", score[BLACK]);

        ut::expect(ut::eq(score[WHITE], -1773));
        ut::expect(ut::eq(score[BLACK], 2204));

        // evaluation with update

        const std::uint16_t removed_features[2][3] = { {
                make_index<WHITE>(A1, C3, WPAWN),
                make_index<WHITE>(A1, D4, BROOK)
            }, {
                make_index<BLACK>(B8, C3, WPAWN),
                make_index<BLACK>(B8, D4, BROOK)
        }};

        const std::uint16_t added_features[2][3] = { {
                make_index<WHITE>(A1, D4, WPAWN)
            }, {
                make_index<BLACK>(B8, D4, WPAWN)
        }};

        nnue.update<WHITE>(accumulator[1], accumulator[0], std::span{removed_features[WHITE]}.first(2), std::span{added_features[WHITE]}.first(1));
        nnue.update<BLACK>(accumulator[1], accumulator[0], std::span{removed_features[BLACK]}.first(2), std::span{added_features[BLACK]}.first(1));

        score[WHITE] = nnue.evaluate<WHITE>(accumulator[1], 3);
        score[BLACK] = nnue.evaluate<BLACK>(accumulator[1], 3);

        // std::printf("%d\n", score[WHITE]);
        // std::printf("%d\n", score[BLACK]);

        ut::expect(ut::eq(score[WHITE], -3));
        ut::expect(ut::eq(score[BLACK], 25));
    };
}

}  // namespace nnue
