#pragma once

#include <searcher.hpp>
#include "ut.hpp"

void test_searcher() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "searcher"_test = [] {
        auto concurrency = std::jthread::hardware_concurrency();
        blocking_input input{"../epd/bkt.txt"};
        std::vector<std::jthread> workers{};
        workers.reserve(concurrency);

        nnue::big_nnue big;
        nnue::small_nnue small;

        constexpr int depth = 12;
        size_t nodes = 0;
        auto t0 = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < concurrency; ++i) {
            workers.emplace_back([&input, &nodes, &big, &small]() {
                char epd[256];
                position_t position{};
                transposition_t transposition{};
                history_t history{position};
                evaluator evaluator{big, small};
                searcher_t searcher{position, transposition, history, evaluator, []() { return false; }};

                while (input.read(epd)) {
                    std::string_view epd_view{epd};
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
            });
        }

        for (auto& worker : workers)
            worker.join();

        auto t1 = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        auto nps = size_t(nodes / (time / 1000.0));

        std::println("nodes = {}, time = {} ms, nps = {}", nodes, time, nps);

        ut::expect(ut::lt(nodes, 194000000));
        // ut::expect(ut::lt(time, 15000));
        // ut::expect(ut::gt(nps, 12000000));
    };
}
