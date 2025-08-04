#pragma once

#include <history.hpp>
#include "ut.hpp"

void test_history() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "history"_test = []() {
        history_t history{};
        move_t move = "e2e4"_m;

        history.put_all(move);
        ut::expect(ut::eq(history.get(move), 0));

        history.put_good(move);
        ut::expect(ut::eq(history.get(move), 50));

        history.put_all(move);
        history.put_good(move);
        ut::expect(ut::eq(history.get(move), 66));

        history.put_all(move);
        ut::expect(ut::eq(history.get(move), 50));

        history.clear();
        ut::expect(ut::eq(history.get(move), 0));
    };
}
