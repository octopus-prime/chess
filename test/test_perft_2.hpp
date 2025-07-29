#pragma once

#include <node.hpp>
#include <position.hpp>
#include "ut.hpp"
#include <print>
#include <cassert>

struct perft_t_2 {
    position_t& position;
    // position_t foo{"4k1q1/8/8/8/5B2/2K5/8/8 w - - 1 2"sv};
    size_t operator()(int depth) noexcept {
        std::array<move2_t, position_t::MAX_MOVES_PER_PLY> buffer;
        std::span<move2_t> moves = position.generate_moves(buffer, bitboards::ALL);

        // std::println("moves:{} {}", moves.size(), moves);
        // std::println("is foo: {}", position == foo);
        // std::println("is foo: {}", std::ranges::equal(position.board, foo.board));
        // std::println("is foo: {}", std::ranges::equal(position.occupied_by_side, foo.occupied_by_side));
        // std::println("is foo: {}", std::ranges::equal(position.occupied_by_type, foo.occupied_by_type));
        // std::println("is foo: {}", std::ranges::equal(position.states, foo.states));
        // std::println("is foo: {}", position.get_side() == foo.get_side());
        // std::println("is foo: {}", position.hash() == foo.hash());
        // std::println("is foo: {}", position.get_material() == foo.get_material());


        if (depth == 0) {
            return 1;
        }

        if (depth == 1) {
            return moves.size();
        }

        return std::ranges::fold_left(moves, size_t{}, [&](size_t acc, const move2_t& move) {
            // auto position_copy = position;
            position.make_move(move);
            acc += (*this)(depth - 1);
            position.undo_move(move);
            // assert(std::ranges::equal(position.board, position_copy.board));
            // assert(std::ranges::equal(position.occupied_by_side, position_copy.occupied_by_side));
            // assert(std::ranges::equal(position.occupied_by_type, position_copy.occupied_by_type));
            // assert(std::ranges::equal(position.states, position_copy.states));
            // assert(position.get_side() == position_copy.get_side());
            // assert(position.hash() == position_copy.hash());
            // assert(position.get_material() == position_copy.get_material());
            // assert(position == position_copy);
            return acc;
        });
    }
};

class blocking_input_2 {
  std::ifstream stream;
  std::mutex mutex;

public:
  blocking_input_2(std::string_view file) : stream{file.data()}, mutex{} {}

  bool read(std::span<char, 256> epd) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!stream.good())
      return false;
    stream.getline(epd.data(), epd.size());
    return true;
  }
};

void test_perft_2() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "perft"_test = [] {
        constexpr int depth = 4;
        auto concurrency = std::jthread::hardware_concurrency();
        blocking_input_2 input{"../epd/perft_long.txt"};
        std::vector<std::jthread> workers{};
        workers.reserve(concurrency);
        for (int i = 0; i < concurrency; ++i) {
            workers.emplace_back([&input]() {
                char epd[256];
                while (input.read(epd)) {
                    std::string_view epd_view{epd};
                    // if (!epd_view.starts_with("r3kbnr/2qn2p1/8/pppBpp1P/3P1Pb1/P1P1P3/1P2Q2P/RNB1K1NR w KQkq -"sv))
                    //     continue;
                    // epd_view = "B3kbnr/2qn2p1/8/ppp1pp1P/3P1Pb1/P1P1P3/1P2Q2P/RNB1K1NR b KQk - 0 1";
                    auto parts = std::views::split(epd_view, ',');
                    auto part = parts.begin();
                    std::string_view pos_part {*part++};
                    position_t position{pos_part};
                    std::advance(part, depth - 1);
                    std::string_view expected_part {*part++};
                    size_t expected = std::atoll(expected_part.data());
                    perft_t_2 perft{position};
                    size_t count = perft(depth);
                    // std::println("{}: {} {}", match[1].str(), count, expected);
                    ut::expect(ut::eq(count, expected)) << pos_part;
                }
            });
        }
    };
}
