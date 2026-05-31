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
        ut::expect(ut::eq(history.get(move, height), 20));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 40));

        for (int i = 0; i < 100; ++i)
            history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 1896));

        history.age();
        ut::expect(ut::eq(history.get(move, height), 189));

        value = 10;

        history.clear();
        ut::expect(ut::eq(history.get(move, height), 0));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 2));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 4));

        for (int i = 0; i < 100; ++i)
            history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 204));

        history.age();
        ut::expect(ut::eq(history.get(move, height), 20));

        height = 10;

        history.clear();
        ut::expect(ut::eq(history.get(move, height), 0));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 1));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 2));

        for (int i = 0; i < 100; ++i)
            history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 102));

        history.age();
        ut::expect(ut::eq(history.get(move, height), 10));

        move = "g1f3"_m;

        history.clear();
        ut::expect(ut::eq(history.get(move, height), 0));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 1));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 2));

        for (int i = 0; i < 100; ++i)
            history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 102));

        history.age();
        ut::expect(ut::eq(history.get(move, height), 10));
    };
}
