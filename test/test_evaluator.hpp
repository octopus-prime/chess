#pragma once

#include <evaluator.hpp>
#include "ut.hpp"

void test_evaluator() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "evaluator"_test = []() {
        position_t position{};
        evaluator evaluator{};

        ut::expect(ut::eq(evaluator.evaluate(position), 6));

        position.make_move("e2e4"_m);
        ut::expect(ut::eq(evaluator.evaluate(position), -23));

        position.make_move("e7e5"_m);
        ut::expect(ut::eq(evaluator.evaluate(position), 10));

        position = "1k6/8/8/8/3r4/2P5/8/K7 w - -"sv;
        ut::expect(ut::eq(evaluator.evaluate(position), -443));

        position = "1k6/8/8/8/3r4/2P5/8/K7 b - -"sv;
        ut::expect(ut::eq(evaluator.evaluate(position), 551));
    };
}
