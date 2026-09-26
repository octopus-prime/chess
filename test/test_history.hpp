#pragma once

#include <history.hpp>
#include "ut.hpp"

void test_history() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "history"_test = []() {
        position_t position{};
        history_t history{position};
        move_t move = "e2e4"_m;
        int height = 3;
        int value = 100;

        ut::expect(ut::eq(history.get(move, height), 0));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 37));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 75));

        for (int i = 0; i < 100; ++i)
            history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 3555));

        history.age();
        ut::expect(ut::eq(history.get(move, height), 355));

        value = 10;

        history.clear();
        ut::expect(ut::eq(history.get(move, height), 0));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 3));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 7));

        for (int i = 0; i < 100; ++i)
            history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 382));

        history.age();
        ut::expect(ut::eq(history.get(move, height), 38));

        height = 10;

        history.clear();
        ut::expect(ut::eq(history.get(move, height), 0));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 1));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 3));

        for (int i = 0; i < 100; ++i)
            history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 191));

        history.age();
        ut::expect(ut::eq(history.get(move, height), 19));

        move = "g1f3"_m;

        history.clear();
        ut::expect(ut::eq(history.get(move, height), 0));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 1));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 3));

        for (int i = 0; i < 100; ++i)
            history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 191));

        history.age();
        ut::expect(ut::eq(history.get(move, height), 19));
    };
}
