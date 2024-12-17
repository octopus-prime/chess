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
        ut::expect(std::string_view{buffer, std::format_to_n(buffer, 7, "{}", move_t{move_t::PAWN, "e2"_s, "e4"_s}).out} == "Pe2e4"sv);
        ut::expect(std::string_view{buffer, std::format_to_n(buffer, 7, "{}", move_t{move_t::CASTLE_SHORT}).out} == "O-O"sv);
        ut::expect(std::string_view{buffer, std::format_to_n(buffer, 7, "{}", move_t{move_t::CASTLE_LONG}).out} == "O-O-O"sv);
    };
}