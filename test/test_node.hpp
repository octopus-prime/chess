#pragma once

#include <node.hpp>
#include "ut.hpp"
#include <print>

static_assert(sizeof(node) == 136);
static_assert(node(std::array{"e2"_b, ""_b}, std::array{""_b, ""_b, ""_b, ""_b, ""_b, ""_b, ""_b, ""_b}, 0, 0).occupied<WHITE>() == "e2"_b);
static_assert(node(std::array{""_b, "e4"_b}, std::array{""_b, ""_b, ""_b, ""_b, ""_b, ""_b, ""_b, ""_b}, 0, 0).occupied<BLACK>() == "e4"_b);
static_assert(node(std::array{"e2"_b, "e4"_b}, std::array{""_b, ""_b, ""_b, ""_b, ""_b, ""_b, ""_b, ""_b}, 0, 0).occupied() == "e2e4"_b);
static_assert(node(std::array{"e2"_b, ""_b}, std::array{""_b, ""_b, ""_b, ""_b, ""_b, ""_b, "e2"_b, ""_b}, 0, 0).king<WHITE>() == "e2"_b);
static_assert(node(std::array{""_b, "e2"_b}, std::array{""_b, ""_b, ""_b, ""_b, "e2"_b, ""_b, ""_b, ""_b}, 0, 0).rook_queen<BLACK>() == "e2"_b);
static_assert(node(std::array{""_b, "e2"_b}, std::array{""_b, ""_b, ""_b, "e2"_b, ""_b, ""_b, ""_b, ""_b}, 0, 0).bishop_queen<BLACK>() == "e2"_b);
static_assert(node(std::array{""_b, "e2"_b}, std::array{""_b, ""_b, "e2"_b, ""_b, ""_b, ""_b, ""_b, ""_b}, 0, 0).knight<BLACK>() == "e2"_b);
static_assert(node(std::array{"e2"_b, ""_b}, std::array{""_b, "e2"_b, ""_b, ""_b, ""_b, ""_b, ""_b, ""_b}, 0, 0).pawn<WHITE>() == "e2"_b);

void test_node() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "node"_test = [] {
        ut::expect(ut::eq(node(std::array{"e2"_b, ""_b}, std::array{""_b, ""_b, ""_b, ""_b, ""_b, ""_b, "e2"_b, ""_b}, 0, 0).attackers<WHITE>(), bitboards::king("e2"_b)));

        ut::expect(ut::eq(node(std::array{""_b, "e2"_b}, std::array{""_b, ""_b, ""_b, "e2"_b, ""_b, ""_b, ""_b, ""_b}, 0, 0).attackers<BLACK>(), bitboards::bishop_queen("e2"_b, 0ull)));
        ut::expect(ut::eq(node(std::array{""_b, "e2"_b}, std::array{""_b, ""_b, ""_b, ""_b, "e2"_b, ""_b, ""_b, ""_b}, 0, 0).attackers<BLACK>(), bitboards::rook_queen("e2"_b, 0ull)));
        ut::expect(ut::eq(node(std::array{""_b, "e2"_b}, std::array{""_b, ""_b, ""_b, ""_b, ""_b, "e2"_b, ""_b, ""_b}, 0, 0).attackers<BLACK>(), bitboards::rook_queen("e2"_b, 0ull) | bitboards::bishop_queen("e2"_b, 0ull)));

        ut::expect(ut::eq(node(std::array{""_b, "e2"_b}, std::array{""_b, ""_b, "e2"_b, ""_b, ""_b, ""_b, ""_b, ""_b}, 0, 0).attackers<BLACK>(), bitboards::knight("e2"_b)));
        ut::expect(ut::eq(node(std::array{"e2"_b, ""_b}, std::array{""_b, "e2"_b, ""_b, ""_b, ""_b, ""_b, ""_b, ""_b}, 0, 0).attackers<WHITE>(), bitboards::pawn<WHITE>("e2"_b)));
    };
}
