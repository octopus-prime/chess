#pragma once

#include <searcher.hpp>
#include "ut.hpp"

void test_searcher() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "searcher"_test = []() {
        position_t position{};
        transposition_t transposition{};
        history_t history{position};
        evaluator evaluator{};
        searcher_t searcher{position, transposition, history, evaluator, []() { return false; }};
        int depth = 9;

        size_t nodes = 0;
        auto t0 = std::chrono::high_resolution_clock::now();

        ut::expect(searcher(depth) == "e2e4"_m);

        searcher.clear();
        position = "1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - -"sv;
        ut::expect(searcher(depth) == "d6d1"_m);
        nodes += searcher.stats.nodes;

        searcher.clear();
        position = "rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq -"sv;
        ut::expect(searcher(depth) == "e5e6"_m);
        nodes += searcher.stats.nodes;

        searcher.clear();
        position = "r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w - -"sv;
        ut::expect(searcher(depth) == "c3d5"_m);
        nodes += searcher.stats.nodes;

        searcher.clear();
        position = "1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w - -"sv;
        ut::expect(searcher(depth) == "h5f6"_m);
        nodes += searcher.stats.nodes;

        searcher.clear();
        position = "4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w - -"sv;
        ut::expect(searcher(depth) == "f4f5"_m);
        nodes += searcher.stats.nodes;

        searcher.clear();
        position = "3rr1k1/pp3pp1/1qn2np1/8/3p4/PP1R1P2/2P1NQPP/R1B3K1 b - -"sv;
        ut::expect(searcher(depth) == "c6e5"_m);
        nodes += searcher.stats.nodes;

        searcher.clear();
        position = "r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1 b - -"sv;
        ut::expect(searcher(depth) == "d7f5"_m);
        nodes += searcher.stats.nodes;

        searcher.clear();
        position = "r2q1rk1/4bppp/p2p4/2pP4/3pP3/3Q4/PP1B1PPP/R3R1K1 w - -"sv;
        ut::expect(searcher(depth) == "b2b4"_m);
        nodes += searcher.stats.nodes;

        searcher.clear();
        position = "2rq1rk1/1b2bp2/p3pn1p/np1p2p1/3P4/PPNQB1P1/3NPPBP/R1R3K1 b - -"sv;
        ut::expect(searcher(depth) == "f6g4"_m);
        nodes += searcher.stats.nodes;

        auto t1 = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        auto nps = size_t(nodes / (time / 1000.0));

        std::println("nodes = {}, time = {} ms, nps = {}", nodes, time, nps);

        ut::expect(ut::le(nodes, 24558728));
        // ut::expect(ut::ge(nps, 2300000));
    };
}
