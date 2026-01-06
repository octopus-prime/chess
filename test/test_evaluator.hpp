#pragma once

#include <evaluator.hpp>
#include "ut.hpp"

void test_evaluator() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    // "evaluator"_test = []() {
    //     position_t position{};
    //     evaluator evaluator{};
    //     int alpha = -30000;
    //     int beta = +30000;

    //     ut::expect(ut::eq(evaluator.evaluate(position, alpha, beta), 5));

    //     position.make_move("e2e4"_m);
    //     ut::expect(ut::eq(evaluator.evaluate(position, alpha, beta), -25));

    //     position.make_move("e7e5"_m);
    //     ut::expect(ut::eq(evaluator.evaluate(position, alpha, beta), 8));

    //     position = "1k6/8/8/8/3r4/2P5/8/K7 w - -"sv;
    //     ut::expect(ut::eq(evaluator.evaluate(position, alpha, beta), -420));

    //     position = "1k6/8/8/8/3r4/2P5/8/K7 b - -"sv;
    //     ut::expect(ut::eq(evaluator.evaluate(position, alpha, beta), 562));
    // };

    "evaluator"_test = []() {
        position_t position{};
        nnue::big_nnue big;
        nnue::small_nnue small;
        evaluator evaluator{big, small};
        int alpha = -30000;
        int beta = +30000;

        ut::expect(ut::eq(evaluator.evaluate(position, alpha, beta), 4));

        position.make_move("e2e4"_m);
        ut::expect(ut::eq(evaluator.evaluate(position, alpha, beta), -24));

        position.make_move("e7e5"_m);
        ut::expect(ut::eq(evaluator.evaluate(position, alpha, beta), 8));

        position = "1k6/8/8/8/3r4/2P5/8/K7 w - -"sv;
        ut::expect(ut::eq(evaluator.evaluate(position, alpha, beta), -420));

        position = "1k6/8/8/8/3r4/2P5/8/K7 b - -"sv;
        ut::expect(ut::eq(evaluator.evaluate(position, alpha, beta), 560));
    };
}
