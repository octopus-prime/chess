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

        constexpr int depth = 12;

        size_t nodes = 0;
        std::ifstream stream("../epd/bkt.txt");
        auto t0 = std::chrono::high_resolution_clock::now();

        while (stream.good()) {
            std::array<char, 256> epd;
            stream.getline(epd.data(), epd.size());
            std::string_view epd_view{epd.data()};

            if (epd_view.empty() || epd_view.starts_with("#")) {
                break;
            }

            auto parts = epd_view | std::views::split("; "sv);
            auto part = parts.begin();

            std::string_view fen_part{*part++};
            position = fen_part;

            std::string_view move_part{*part++};
            move_t move{move_part};

            auto result = searcher(depth);
            ut::expect(result == move) << epd_view;

            nodes += searcher.stats.nodes;
            searcher.clear();
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        auto nps = size_t(nodes / (time / 1000.0));

        std::println("nodes = {}, time = {} ms, nps = {}", nodes, time, nps);

        ut::expect(ut::lt(nodes, 100000000));
        // ut::expect(ut::lt(time, 46000));
        // ut::expect(ut::gt(nps, 2170000));
    };
}
