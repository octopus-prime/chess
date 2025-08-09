#pragma once

#include <move_picker.hpp>
#include "ut.hpp"

void test_move_picker() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "move_picker"_test = []() {
        position_t position{"4k3/6p1/3p4/8/4N3/8/3P4/4K3 w - -"sv};
        killer_t::entry_t killers{"e1e2"_m, "e1f2"_m};
        move_t best = "d2d4"_m;
        history_t history{};
        auto history_moves = { "e1d1"_m, "e1f1"_m };
        for (const auto& move : history_moves) {
            history.put_all(move);
            history.put_good(move);
        }

        std::array<move_t, position_t::MAX_MOVES_PER_PLY> buffer;
        auto moves = position.generate_moves(buffer, bitboards::ALL);
        move_picker_t move_picker{position, history, killers, best, moves};

        auto tt_moves = move_picker(move_picker_t::TT_MOVES);
        ut::expect(std::ranges::equal(tt_moves | std::views::elements<0>, std::views::single(best)));

        auto good_capture_moves = move_picker(move_picker_t::GOOD_CAPTURE_MOVES);
        ut::expect(std::ranges::equal(good_capture_moves | std::views::elements<0>, std::views::single("e4d6"_m)));
        ut::expect(std::ranges::all_of(good_capture_moves | std::views::elements<1>, [](int see_eval) {
            return see_eval  == +100;
        }));

        auto quiet_moves = move_picker(move_picker_t::QUIET_MOVES);
        ut::expect(std::ranges::is_permutation(quiet_moves | std::views::elements<0> | std::views::take(2), killers));
        ut::expect(std::ranges::is_permutation(quiet_moves | std::views::elements<0> | std::views::drop(2) | std::views::take(2), history_moves));
        ut::expect(std::ranges::is_permutation(quiet_moves | std::views::elements<0> | std::views::drop(4), std::initializer_list{"d2d3"_m, "e4c3"_m, "e4g5"_m, "e4g3"_m, "e4f2"_m}));
        ut::expect(std::ranges::all_of(quiet_moves | std::views::elements<1>, [](int see_eval) {
            return see_eval  == 0;
        }));

        auto bad_capture_moves = move_picker(move_picker_t::BAD_CAPTURE_MOVES);
        ut::expect(std::ranges::is_permutation(bad_capture_moves | std::views::elements<0>, std::initializer_list{"e4c5"_m, "e4f6"_m}));
        ut::expect(std::ranges::all_of(bad_capture_moves | std::views::elements<1>, [](int see_eval) {
            return see_eval  == -300;
        }));
    };
}
