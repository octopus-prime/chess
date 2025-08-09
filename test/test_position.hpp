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
}
