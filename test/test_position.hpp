#pragma once

#include <position.hpp>
#include "ut.hpp"

void test_position() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    // note: perft tests

    "check"_test = [] {
        ut::expect(ut::eq(position_t{"4kb2/8/8/8/8/8/8/4KN2 w - -"}.is_check(), false));
        ut::expect(ut::eq(position_t{"2Q3n1/k2R4/p7/5p2/1pN5/5P2/PPP5/2K1R3 b - -"}.is_check(), true));
    };

    "move_check"_test = [] {
        position_t position{"5rk1/p2b4/P7/3n4/8/2p5/1P2P1q1/3K4 b - -"};
        ut::expect(ut::eq(position.check("g2e2"_m), true));
        ut::expect(ut::eq(position.check("g2f1"_m), true));
        ut::expect(ut::eq(position.check("g2g1"_m), true));
        ut::expect(ut::eq(position.check("g2h1"_m), true));
        ut::expect(ut::eq(position.check("g2f3"_m), false));
        ut::expect(ut::eq(position.check("f8f1"_m), true));
        ut::expect(ut::eq(position.check("f8d8"_m), false));
        ut::expect(ut::eq(position.check("d7a4"_m), true));
        ut::expect(ut::eq(position.check("d7g4"_m), false));
        ut::expect(ut::eq(position.check("d5e3"_m), true));
        ut::expect(ut::eq(position.check("d5b4"_m), false));
        ut::expect(ut::eq(position.check("c3c2"_m), true));
        ut::expect(ut::eq(position.check("c3b2"_m), false));
        position = "3k4/1p2p1Q1/2P5/8/3N4/p7/P2B4/5RK1 w - -"sv;
        ut::expect(ut::eq(position.check("g7e7"_m), true));
        ut::expect(ut::eq(position.check("g7f8"_m), true));
        ut::expect(ut::eq(position.check("g7g8"_m), true));
        ut::expect(ut::eq(position.check("g7h8"_m), true));
        ut::expect(ut::eq(position.check("g7f6"_m), false));
        ut::expect(ut::eq(position.check("f1f8"_m), true));
        ut::expect(ut::eq(position.check("f1d1"_m), false));
        ut::expect(ut::eq(position.check("d2a5"_m), true));
        ut::expect(ut::eq(position.check("d2g5"_m), false));
        ut::expect(ut::eq(position.check("d4e6"_m), true));
        ut::expect(ut::eq(position.check("d4b5"_m), false));
        ut::expect(ut::eq(position.check("c6c7"_m), true));
        ut::expect(ut::eq(position.check("c6b7"_m), false));
    };

    "50_moves_rule"_test = [] {
        ut::expect(ut::eq(position_t{"4kb2/8/8/8/8/8/8/4KN2 w - - 99 123"}.is_50_moves_rule(), false));
        ut::expect(ut::eq(position_t{"4kb2/8/8/8/8/8/8/4KN2 w - - 100 123"}.is_50_moves_rule(), true));
    };

    "3_fold_repetition"_test = [] {
        position_t position{};
        position.make_move("g1f3"_m);
        position.make_move("g8f6"_m);
        position.make_move("f3g1"_m);
        position.make_move("f6g8"_m);
        position.make_move("g1f3"_m);
        position.make_move("g8f6"_m);
        position.make_move("f3g1"_m);
        ut::expect(ut::eq(position.is_3_fold_repetition(), false)); 
        position.make_move("f6g8"_m);
        ut::expect(ut::eq(position.is_3_fold_repetition(), true));
    };

    "no_material"_test = [] {
        ut::expect(ut::eq(position_t{}.is_no_material(), false));
        ut::expect(ut::eq(position_t{"4kb2/8/8/8/8/8/8/4KN2 w - -"}.is_no_material(), true));
    };

    "see"_test = []() {
        std::ifstream stream{"../epd/see.txt"};
        std::array<char, 256> epd;
        position_t position;
        while (stream.good()) {
            stream.getline(epd.data(), epd.size());
            std::string_view epd_view{epd.data()};

            if (epd_view.empty() || epd_view.starts_with("#")) {
                return;
            }

            auto parts = epd_view | std::views::split("; "sv);
            auto part = parts.begin();

            std::string_view fen_part{*part++};
            position = fen_part;

            std::string_view move_part{*part++};
            move_t move{move_part};

            std::string_view see_part{*part++};
            int see_value;
            std::from_chars(&*see_part.begin(), &*see_part.end(), see_value);

            int see_result = position.see(move);
            ut::expect(ut::eq(see_result, see_value)) << epd_view;
            // std::println("{}: {} -> {}", epd_view, move, see_value);
        }
    };

    "generate_active_moves"_test = [] {
        position_t position{"4kb2/8/8/8/8/8/8/4KN2 w - -"sv};
        std::array<move_t, position_t::MAX_ACTIVE_MOVES_PER_PLY> buffer;
        auto moves = position.generate_active_moves(buffer);
        ut::expect(std::ranges::is_permutation(moves, std::views::empty<move_t>));

        position = "4k3/7P/1N6/3p4/8/8/8/4K3 w - -"sv;
        moves = position.generate_active_moves(buffer);
        ut::expect(std::ranges::is_permutation(moves, std::initializer_list{"b6d5"_m, "h7h8q"_m, "h7h8n"_m}));

        position = "4k1n1/7P/1N6/3p4/8/8/8/4K3 w - -"sv;
        moves = position.generate_active_moves(buffer);
        ut::expect(std::ranges::is_permutation(moves, std::initializer_list{"b6d5"_m, "h7h8q"_m, "h7h8n"_m, "h7g8q"_m, "h7g8n"_m}));

        position = "4k1n1/7P/1N6/3p3N/8/8/8/R2BK3 w - -"sv;
        moves = position.generate_active_moves(buffer);
        ut::expect(std::ranges::is_permutation(moves, std::initializer_list{"b6d5"_m, "h7h8q"_m, "h7h8n"_m, "h7g8q"_m, "h7g8n"_m, "a1a8"_m, "d1a4"_m, "h5g7"_m, "h5f6"_m}));

        position = "4k1nQ/7P/1N1P4/3p3N/8/8/8/R2BK3 w - -"sv;
        moves = position.generate_active_moves(buffer);
        ut::expect(std::ranges::is_permutation(moves, std::initializer_list{"h5f6"_m, "h5g7"_m, "b6d5"_m, "a1a8"_m, "h8g8"_m, "d1a4"_m, "h8e5"_m, "d6d7"_m, "h7g8q"_m, "h7g8n"_m}));

        position = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"sv;
        moves = position.generate_active_moves(buffer);
        ut::expect(std::ranges::is_permutation(moves, std::initializer_list{"g2h3"_m, "f3h3"_m, "f3f6"_m, "e5g6"_m, "e5f7"_m, "e5d7"_m, "d5e6"_m, "e2a6"_m}));

        // position = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -"sv;
        // moves = position.generate_active_moves(buffer);
        // ut::expect(ut::eq(moves.size(), 5));
        // ut::expect(std::ranges::is_permutation(moves, std::initializer_list{"g2h3"_m, "f3h3"_m, "f3f6"_m, "e5g6"_m, "e5f7"_m}));
    };
}
