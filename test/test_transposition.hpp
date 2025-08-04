#pragma once

#include <transposition.hpp>
#include "ut.hpp"

static_assert(sizeof(entry_t) == 16);

void test_transposition() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "transposition"_test = []() {
        transposition_t transposition{};

        hash_t hash = 0x1234567890abcdef;
        move_t move = "e2e4"_m;
        int score = 100;
        flag_t flag = EXACT;
        int depth = 10;

        transposition.put(hash, move, score, flag, depth);

        const entry_t* entry = transposition.get(hash);
        ut::expect(entry != nullptr);
        ut::expect(ut::eq(entry->hash, hash));
        ut::expect(entry->move == move);
        ut::expect(ut::eq(entry->score, score));
        ut::expect(ut::eq(entry->flag, flag));
        ut::expect(ut::eq(entry->depth, depth));

        transposition.clear();
        entry = transposition.get(hash);
        ut::expect(entry == nullptr);
    };
}
