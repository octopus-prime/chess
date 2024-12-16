#pragma once

#include <bitboard.hpp>
#include "ut.hpp"

static_assert(sizeof(bitboard) == 8);
static_assert("b3f7"_b.empty() == false);
static_assert("b3f7"_b.front() == b3);
static_assert("b3f7"_b.back() == "f7"_s);
static_assert("b3f7"_b.size() == 2);
static_assert(("b"_f & "3"_r) == "b3"_b);

static_assert(std::input_iterator<bitboard::iterator>);
static_assert(std::ranges::input_range<bitboard>);
static_assert(std::ranges::sized_range<bitboard>);

// #include <print>
// void demo_bitboard() {
//     constexpr auto bb = "b3f7"_b;
//     for (square s : bb) {
//         std::println("{}", s);
//     }
// }

void test_bitboard() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "bitboard"_test = [] {
        char buffer[72];
        ut::expect(std::string_view{buffer, std::format_to_n(buffer, 72, "{}", "b3f7"_b).out} == "b3f7"sv);
        ut::expect(std::string_view{buffer, std::format_to_n(buffer, 72, "{:b}", "b3f7"_b).out} == ""
            "00000000\n"
            "00000100\n"
            "00000000\n"
            "00000000\n"
            "00000000\n"
            "01000000\n"
            "00000000\n"
            "00000000\n"sv
        );

    };
}