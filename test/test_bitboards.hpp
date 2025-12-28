#pragma once

#include <bitboards.hpp>
#include "ut.hpp"

void test_bitboards() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "bitboards king"_test = [] {
        ut::expect(ut::eq(bitboards::king("e4"_s), "d3e3f3d4f4d5e5f5"_b));
        ut::expect(ut::eq(bitboards::leaper("e4"_b, ""_b), "d3e3f3d4f4d5e5f5"_b));
    };

    "bitboards knight"_test = [] {
        ut::expect(ut::eq(bitboards::knight("e4"_s), "d2f2c3g3c5g5d6f6"_b));
        ut::expect(ut::eq(bitboards::leaper(""_b, "e4"_b), "d2f2c3g3c5g5d6f6"_b));
    };

    "bitboards rook_queen"_test = [] {
        ut::expect(ut::eq(bitboards::rook_queen("e4"_s, ""_b), "e1e2e3a4b4c4d4f4g4h4e5e6e7e8"_b));
        ut::expect(ut::eq(bitboards::slider("e4"_b, ""_b, ""_b), "e1e2e3a4b4c4d4f4g4h4e5e6e7e8"_b));
    };

    "bitboards bishop_queen"_test = [] {
        ut::expect(ut::eq(bitboards::bishop_queen("e4"_s, ""_b), "b1h1c2g2d3f3d5f5c6g6b7h7a8"_b));
        ut::expect(ut::eq(bitboards::slider(""_b, "e4"_b, ""_b), "b1h1c2g2d3f3d5f5c6g6b7h7a8"_b));
    };

    "bitboards pawn"_test = [] {
        ut::expect(ut::eq(bitboards::pawn<WHITE>("e4"_s), "d5f5"_b));
        ut::expect(ut::eq(bitboards::pawn<WHITE>("e4"_b), "d5f5"_b));
        ut::expect(ut::eq(bitboards::pawn<BLACK>("e4"_s), "d3f3"_b));
        ut::expect(ut::eq(bitboards::pawn<BLACK>("e4"_b), "d3f3"_b));
    };

    "bitboards line"_test = [] {
        ut::expect(ut::eq(bitboards::line("e4"_s, "a8"_s), "e4d5c6b7a8"_b));
        ut::expect(ut::eq(bitboards::line("e4"_s, "e1"_s), "e4e3e2e1"_b));
        ut::expect(ut::eq(bitboards::line("e4"_s, "b8"_s), "e4b8"_b));
        ut::expect(ut::eq(bitboards::line("e4"_s, "f1"_s), "e4f1"_b));
    };

    "bitboards ray"_test = [] {
        ut::expect(ut::eq(bitboards::ray("e4"_s, NORTH), "e5e6e7e8"_b));
        ut::expect(ut::eq(bitboards::ray("e4"_s, SOUTH), "e3e2e1"_b));
        ut::expect(ut::eq(bitboards::ray("e4"_s, EAST), "f4g4h4"_b));
        ut::expect(ut::eq(bitboards::ray("e4"_s, WEST), "d4c4b4a4"_b));
        ut::expect(ut::eq(bitboards::ray("e4"_s, NORTH_EAST), "f5g6h7"_b));
        ut::expect(ut::eq(bitboards::ray("e4"_s, NORTH_WEST), "d5c6b7a8"_b));
        ut::expect(ut::eq(bitboards::ray("e4"_s, SOUTH_EAST), "f3g2h1"_b));
        ut::expect(ut::eq(bitboards::ray("e4"_s, SOUTH_WEST), "d3c2b1"_b));
    };
}
