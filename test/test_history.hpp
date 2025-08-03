#pragma once

#include <history.hpp>
#include "ut.hpp"

void test_history() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "history"_test = []() {
        history_t history{};

        history.put_all("e2e4"_m, 8, WHITE);
        ut::expect(ut::eq(history.get("e2e4"_m, 8, WHITE), 0));

        history.put_good("e2e4"_m, 8, WHITE);
        ut::expect(ut::eq(history.get("e2e4"_m, 8, WHITE), 50));

        history.put_all("e2e4"_m, 8, WHITE);
        history.put_good("e2e4"_m, 8, WHITE);
        ut::expect(ut::eq(history.get("e2e4"_m, 8, WHITE), 66));

        history.put_all("e2e4"_m, 8, WHITE);
        ut::expect(ut::eq(history.get("e2e4"_m, 8, WHITE), 50));
    };
}
