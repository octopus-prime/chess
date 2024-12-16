#pragma once

#include <square.hpp>
#include "ut.hpp"

static_assert(sizeof(square) == 1);
static_assert(square{"a1"} == a1);
static_assert(square{"h8"} == h8);
static_assert(square{"b3"} == "b3"_s);
static_assert(square{f_b, r_3} == "b3"_s);
static_assert(square{"b3"}.file() == f_b);
static_assert(square{"b3"}.rank() == r_3);
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
        ut::expect(std::string_view{buffer, std::format_to_n(buffer, 2, "{}", "b3"_s).out} == "b3"sv);
    };
}
