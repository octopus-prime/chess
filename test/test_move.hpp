#pragma once

#include <move.hpp>
#include "ut.hpp"

static_assert(sizeof(move_t) == 3);
static_assert("e2e4"_m.from() == E2);
static_assert("e2e4"_m.to() == E4);
static_assert("e7e8q"_m.promotion() == QUEEN);

void test_move() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "move"_test = []() {
        ut::expect("e2e4"_m == move_t{E2, E4, ""_t});
        ut::expect("e7e8p"_m == move_t{E7, E8, "p"_t});
        ut::expect("e7e8q"_m == move_t{E7, E8, "q"_t});
    };
}