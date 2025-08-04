#pragma once

#include <killer.hpp>
#include "ut.hpp"

void test_killer() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "killer"_test = []() {
        killer_t killer{};
        move_t move1 = "e2e4"_m;
        move_t move2 = "d2d4"_m;
        move_t move3 = "g1f3"_m;
        int height = 10;

        killer.put(move1, height);
        ut::expect(std::ranges::contains(killer.get(height), move1));

        killer.put(move2, height);
        ut::expect(std::ranges::contains(killer.get(height), move1));
        ut::expect(std::ranges::contains(killer.get(height), move2));

        killer.put(move3, height);
        ut::expect(std::ranges::contains(killer.get(height), move2));
        ut::expect(std::ranges::contains(killer.get(height), move3));

        killer.clear();
        ut::expect(!std::ranges::contains(killer.get(height), move1));
        ut::expect(!std::ranges::contains(killer.get(height), move2));
        ut::expect(!std::ranges::contains(killer.get(height), move3));
    };
}
