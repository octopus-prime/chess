#pragma once

#include <position.hpp>
#include "ut.hpp"
// #include "blocking_input.hpp"
#include <print>
#include <cassert>

struct perft_t {
    position_t& position;

    size_t operator()(int depth) noexcept {
        std::array<move_t, position_t::MAX_MOVES_PER_PLY> buffer;
        std::span<move_t> moves = position.generate_all_moves(buffer);

        if (depth == 0) {
            return 1;
        }

        if (depth == 1) {
            return moves.size();
        }

        return std::ranges::fold_left(moves, size_t{}, [&](size_t acc, const move_t& move) {
            position.make_move(move);
            acc += (*this)(depth - 1);
            position.undo_move(move);
            return acc;
        });
    }
};

void test_perft() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "perft"_test = [] {
        auto concurrency = std::jthread::hardware_concurrency();
        blocking_input input{"../epd/perft_long.txt"};
        std::vector<std::jthread> workers{};
        workers.reserve(concurrency);

        constexpr int depth = 4;
        std::size_t nodes = 0;
        auto t0 = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < concurrency; ++i) {
            workers.emplace_back([&input, &nodes]() {
                char epd[256];
                position_t position;
                while (input.read(epd)) {
                    std::string_view epd_view{epd};
                    auto parts = std::views::split(epd_view, ',');
                    auto part = parts.begin();
                    std::string_view pos_part {*part++};
                    position = pos_part;
                    std::advance(part, depth - 1);
                    std::string_view expected_part {*part++};
                    size_t expected = std::atoll(expected_part.data());
                    perft_t perft{position};
                    size_t count = perft(depth);
                    ut::expect(ut::eq(count, expected)) << pos_part;
                    nodes += count;
                }
            });
        }

        for (auto& worker : workers)
            worker.join();

        auto t1 = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        auto nps = size_t(nodes / (time / 1000.0));

        std::println("nodes = {}, time = {} ms, nps = {}", nodes, time, nps);
    };
}
