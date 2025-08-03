#pragma once

#include <position.hpp>
#include "ut.hpp"
#include <print>
#include <cassert>

struct perft_t {
    position_t& position;

    size_t operator()(int depth) noexcept {
        std::array<move_t, position_t::MAX_MOVES_PER_PLY> buffer;
        std::span<move_t> moves = position.generate_moves(buffer, bitboards::ALL);

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

class blocking_input {
  std::ifstream stream;
  std::mutex mutex;

public:
  blocking_input(std::string_view file) : stream{file.data()}, mutex{} {}

  bool read(std::span<char, 256> epd) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!stream.good())
      return false;
    stream.getline(epd.data(), epd.size());
    return true;
  }
};

void test_perft() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "perft"_test = [] {
        constexpr int depth = 4;
        auto concurrency = std::jthread::hardware_concurrency();
        blocking_input input{"../epd/perft_long.txt"};
        std::vector<std::jthread> workers{};
        workers.reserve(concurrency);
        for (int i = 0; i < concurrency; ++i) {
            workers.emplace_back([&input]() {
                char epd[256];
                while (input.read(epd)) {
                    std::string_view epd_view{epd};
                    auto parts = std::views::split(epd_view, ',');
                    auto part = parts.begin();
                    std::string_view pos_part {*part++};
                    position_t position{pos_part};
                    std::advance(part, depth - 1);
                    std::string_view expected_part {*part++};
                    size_t expected = std::atoll(expected_part.data());
                    perft_t perft{position};
                    size_t count = perft(depth);
                    ut::expect(ut::eq(count, expected)) << pos_part;
                }
            });
        }
    };
}
