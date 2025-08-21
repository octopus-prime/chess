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
        ut::expect(ut::eq(history.get(move, height), 50));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 100));

        for (int i = 0; i < 100; ++i)
            history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 4741));

        history.age();
        ut::expect(ut::eq(history.get(move, height), 474));

        value = 10;

        history.clear();
        ut::expect(ut::eq(history.get(move, height), 0));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 5));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 10));

        for (int i = 0; i < 100; ++i)
            history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 510));

        history.age();
        ut::expect(ut::eq(history.get(move, height), 51));

        height = 10;

        history.clear();
        ut::expect(ut::eq(history.get(move, height), 0));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 2));

        history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 5));

        for (int i = 0; i < 100; ++i)
            history.put(move, height, value);
        ut::expect(ut::eq(history.get(move, height), 255));

        history.age();
        ut::expect(ut::eq(history.get(move, height), 25));

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
