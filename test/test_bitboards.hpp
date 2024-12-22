#pragma once

#include <bitboards.hpp>
#include "ut.hpp"

void test_bitboards() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "bitboards"_test = [] {
        ut::expect(ut::eq(bitboards::king("e4"_s), "d3e3f3d4f4d5e5f5"_b));
        ut::expect(ut::eq(bitboards::king("e4"_b), "d3e3f3d4f4d5e5f5"_b));
        ut::expect(ut::eq(bitboards::knight("e4"_s), "d2f2c3g3c5g5d6f6"_b));
        ut::expect(ut::eq(bitboards::knight("e4"_b), "d2f2c3g3c5g5d6f6"_b));
        ut::expect(ut::eq(bitboards::rook_queen("e4"_s, ""_b), "e1e2e3a4b4c4d4f4g4h4e5e6e7e8"_b));
        ut::expect(ut::eq(bitboards::rook_queen("e4"_b, ""_b), "e1e2e3a4b4c4d4f4g4h4e5e6e7e8"_b));
        ut::expect(ut::eq(bitboards::bishop_queen("e4"_s, ""_b), "b1h1c2g2d3f3d5f5c6g6b7h7a8"_b));
        ut::expect(ut::eq(bitboards::bishop_queen("e4"_b, ""_b), "b1h1c2g2d3f3d5f5c6g6b7h7a8"_b));
        ut::expect(ut::eq(bitboards::pawn<WHITE>("e4"_s), "d5f5"_b));
        ut::expect(ut::eq(bitboards::pawn<WHITE>("e4"_b), "d5f5"_b));
        ut::expect(ut::eq(bitboards::pawn<BLACK>("e4"_s), "d3f3"_b));
        ut::expect(ut::eq(bitboards::pawn<BLACK>("e4"_b), "d3f3"_b));
    };
}
