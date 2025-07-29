#pragma once

#include <position.hpp>
#include "ut.hpp"

void test_position() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    struct test_data {
        std::string_view fen;
        bitboard wpawn, wknight, wbishop, wrook, wqueen, wking;
        bitboard bpawn, bknight, bbishop, brook, bqueen, bking;
        bitboard castle, en_passant;
        side_e side;
        bool check, rule50;
    };

    "position"_test = [] {
        test_data tests[] = {
            {
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"sv, 
                "a2b2c2d2e2f2g2h2"_b, "b1g1"_b, "c1f1"_b, "a1h1"_b, "d1"_b, "e1"_b,
                "a7b7c7d7e7f7g7h7"_b, "b8g8"_b, "c8f8"_b, "a8h8"_b, "d8"_b, "e8"_b,
                "a1h1a8h8"_b, ""_b,
                WHITE, false, false
            },
            {
                "r2r2k1/1p1q1pp1/2n3bp/p3p3/P3P3/1P1P1P1P/2P2NP1/R1BQR1K1 w - - 100 70"sv,
                "a4b3c2d3e4f3g2h3"_b, "f2"_b, "c1"_b, "a1e1"_b, "d1"_b, "g1"_b,
                "a5b7e5f7g7h6"_b, "c6"_b, "g6"_b, "a8d8"_b, "d7"_b, "g8"_b,
                ""_b, ""_b,
                WHITE, false, true
            },
            {
                "7k/8/8/8/8/8/8/4K2R b K - 13 37"sv,
                ""_b, ""_b, ""_b, "h1"_b, ""_b, "e1"_b,
                ""_b, ""_b, ""_b, ""_b, ""_b, "h8"_b,
                "h1"_b, ""_b,
                BLACK, true, false
            },
            {
                "4k3/8/8/8/4Pp2/8/8/4K3 b - e3 99 99"sv,
                "e4"_b, ""_b, ""_b, ""_b, ""_b, "e1"_b,
                "f4"_b, ""_b, ""_b, ""_b, ""_b, "e8"_b,
                ""_b, "e3"_b,
                BLACK, false, false
            },
        };

        for (const auto& [fen, wpawn, wknight, wbishop, wrook, wqueen, wking, bpawn, bknight, bbishop, brook, bqueen, bking, castle, en_passant, side, check, rule50] : tests) {
            position_t position{fen};
            ut::expect(ut::eq(position.by(WPAWN), wpawn));
            ut::expect(ut::eq(position.by(WKNIGHT), wknight));
            ut::expect(ut::eq(position.by(WBISHOP), wbishop));
            ut::expect(ut::eq(position.by(WROOK), wrook));
            ut::expect(ut::eq(position.by(WQUEEN), wqueen));
            ut::expect(ut::eq(position.by(WKING), wking));
            ut::expect(ut::eq(position.by(WHITE), wking | wqueen | wrook | wbishop | wknight | wpawn));
            ut::expect(ut::eq(position.by(BPAWN), bpawn));
            ut::expect(ut::eq(position.by(BKNIGHT), bknight));
            ut::expect(ut::eq(position.by(BBISHOP), bbishop));
            ut::expect(ut::eq(position.by(BROOK), brook));
            ut::expect(ut::eq(position.by(BQUEEN), bqueen));
            ut::expect(ut::eq(position.by(BKING), bking));
            ut::expect(ut::eq(position.by(BLACK), bking | bqueen | brook | bbishop | bknight | bpawn));
            ut::expect(ut::eq(position.by(PAWN), wpawn | bpawn));
            ut::expect(ut::eq(position.by(KNIGHT), wknight | bknight));
            ut::expect(ut::eq(position.by(BISHOP), wbishop | bbishop));
            ut::expect(ut::eq(position.by(ROOK), wrook | brook));
            ut::expect(ut::eq(position.by(QUEEN), wqueen | bqueen));
            ut::expect(ut::eq(position.by(KING), wking | bking));
            // ut::expect(ut::eq(position.castle(), castle));
            // ut::expect(ut::eq(position.en_passant(), en_passant));
            ut::expect(ut::eq(position.get_side(), side));
            ut::expect(ut::eq(position.is_check(), check)) << fen;
            ut::expect(ut::eq(position.is_50_moves_rule(), rule50));
        }
    };

    // "position 3_fold_repetition"_test = [] {
    //     position_t position{};
    //     move2_t move{};
    //     position.make_move(move);
    //     ut::expect(ut::eq(position.is_3_fold_repetition(), false));
    //     position.make_move(move);
    //     ut::expect(ut::eq(position.is_3_fold_repetition(), true));
    //     position.undo_move(move);
    //     ut::expect(ut::eq(position.is_3_fold_repetition(), false));
    // };

    "foo"_test = []() {
        ut::expect("e2e4"_m == move2_t{E2, E4, ""_t});
        ut::expect("e7e8p"_m == move2_t{E7, E8, "p"_t});
        ut::expect("e7e8q"_m == move2_t{E7, E8, "q"_t});
    };
}
