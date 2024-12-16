#pragma once

#include <nnue/nnue.hpp>

namespace nnue {

void test_nnue() {
    using NNUE = big_nnue;

    const NNUE nnue;

    const auto version = nnue.version();
    const auto hash = nnue.hash();
    const auto description = nnue.description();

    std::printf("%d\n", version);
    std::printf("%d\n", hash);
    std::printf("%s\n", description.data());

    if (version != 2062757664)
        throw std::runtime_error{"test_nnue (version) failed"};

    if (hash != 470819058)
        throw std::runtime_error{"test_nnue (hash) failed"};
    
    if (description != "u0pi21KGe9rz0Ed with the https://github.com/official-stockfish/nnue-pytorch trainer."sv)
        throw std::runtime_error{"test_nnue (description) failed"};

    NNUE::Accumulator accumulator[2];
    std::int32_t score[2];

    // evaluation with refresh

    const std::uint16_t active_features[2][32] = { {
            make_index<WHITE>(SQ_A1, SQ_A1, W_KING),
            make_index<WHITE>(SQ_A1, SQ_B8, B_KING),
            make_index<WHITE>(SQ_A1, SQ_C3, W_PAWN),
            make_index<WHITE>(SQ_A1, SQ_D4, B_ROOK)
        }, {
            make_index<BLACK>(SQ_B8, SQ_A1, W_KING),
            make_index<BLACK>(SQ_B8, SQ_B8, B_KING),
            make_index<BLACK>(SQ_B8, SQ_C3, W_PAWN),
            make_index<BLACK>(SQ_B8, SQ_D4, B_ROOK)
        }
    };

    nnue.refresh<WHITE>(accumulator[0], std::span{active_features[WHITE]}.first(4));
    nnue.refresh<BLACK>(accumulator[0], std::span{active_features[BLACK]}.first(4));

    score[WHITE] = nnue.evaluate<WHITE>(accumulator[0], 4);
    score[BLACK] = nnue.evaluate<BLACK>(accumulator[0], 4);

    std::printf("%d\n", score[WHITE]);
    std::printf("%d\n", score[BLACK]);

    if (score[WHITE] != -887)
        throw std::runtime_error{"test_nnue (white) failed"};

    if (score[BLACK] != 1317)
        throw std::runtime_error{"test_nnue (black) failed"};

    // evaluation with update

    const std::uint16_t removed_features[2][3] = { {
            make_index<WHITE>(SQ_A1, SQ_C3, W_PAWN),
            make_index<WHITE>(SQ_A1, SQ_D4, B_ROOK)
        }, {
            make_index<BLACK>(SQ_B8, SQ_C3, W_PAWN),
            make_index<BLACK>(SQ_B8, SQ_D4, B_ROOK)
    }};

    const std::uint16_t added_features[2][3] = { {
            make_index<WHITE>(SQ_A1, SQ_D4, W_PAWN)
        }, {
            make_index<BLACK>(SQ_B8, SQ_D4, W_PAWN)
    }};

    nnue.update<WHITE>(accumulator[1], accumulator[0], std::span{removed_features[WHITE]}.first(2), std::span{added_features[WHITE]}.first(1));
    nnue.update<BLACK>(accumulator[1], accumulator[0], std::span{removed_features[BLACK]}.first(2), std::span{added_features[BLACK]}.first(1));

    score[WHITE] = nnue.evaluate<WHITE>(accumulator[1], 3);
    score[BLACK] = nnue.evaluate<BLACK>(accumulator[1], 3);

    std::printf("%d\n", score[WHITE]);
    std::printf("%d\n", score[BLACK]);

    if (score[WHITE] != 135)
        throw std::runtime_error{"test_nnue (white) failed"};

    if (score[BLACK] != -113)
        throw std::runtime_error{"test_nnue (black) failed"};
}

}  // namespace nnue
