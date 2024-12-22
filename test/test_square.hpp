#pragma once

#include <square.hpp>
#include "ut.hpp"

static_assert(sizeof(square) == 1);
static_assert(square{"a1"} == A1);
static_assert(square{"h8"} == H8);
static_assert(square{"b3"} == "b3"_s);
static_assert(square{FB, R3} == "b3"_s);
static_assert(square{"b3"}.file() == FB);
static_assert(square{"b3"}.rank() == R3);
static_assert(square{"b3"} + 1 == "c3"_s);

// void demo_square() {
//     std::println("square = {}", "b3"_s);
//     for (square s : enum_range(a8, h8)) {
//         std::println("square = {}", s);
//     }
// }

void test_square() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "square"_test = [] {
        char buffer[2];
        ut::expect(ut::eq(std::string_view{buffer, std::format_to_n(buffer, 2, "{}", "b3"_s).out}, "b3"sv));
    };
}
