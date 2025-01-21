#pragma once

#include <move.hpp>
#include "ut.hpp"

static_assert(sizeof(move_t) == 3);
static_assert(move_t{move_t::PAWN, "e2"_s, "e4"_s}.from() == "e2"_s);
static_assert(move_t{move_t::PAWN, "e2"_s, "e4"_s}.to() == "e4"_s);
static_assert(move_t{move_t::PAWN, "e2"_s, "e4"_s}.type() == move_t::PAWN);

void test_move() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "move"_test = [] {
        char buffer[7];
        ut::expect(ut::eq(std::string_view{buffer, std::format_to_n(buffer, 7, "{}", move_t{move_t::PAWN, "e2"_s, "e4"_s}).out}, "e2e4"sv));
        ut::expect(ut::eq(std::string_view{buffer, std::format_to_n(buffer, 7, "{}", move_t{move_t::CASTLE_SHORT, "e1"_s, "g1"_s}).out}, "e1g1"sv));
        ut::expect(ut::eq(std::string_view{buffer, std::format_to_n(buffer, 7, "{}", move_t{move_t::CASTLE_LONG, "e8"_s, "c8"_s}).out}, "e8c8"sv));
    };
}