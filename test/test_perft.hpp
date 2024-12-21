#pragma once

#include <node.hpp>
#include "ut.hpp"

template <side_e side>
std::size_t perft(const node &current, int depth) noexcept
{
    std::size_t count = 0;
    std::array<move_t, 256> buffer;
    auto moves = current.generate<side, node::all>(buffer);
    for (const auto &move : moves)
    {
        std::size_t count_ = 0;
        if (depth <= 1)
            count_ += 1;
        else
        {
            node succ(current);
            succ.execute<side>(move);
            std::array<move_t, 256> buffer2;
            count_ += depth == 2 ? succ.generate<~side, node::all>(buffer2).size() : perft<~side>(succ, depth - 1);
        }
        count += count_;
    }
    return count;
}

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
    static const std::regex epd_regex("(.*),(.*),(.*),(.*),(.*),(.*),(.*)");

    namespace ut = boost::ut;
    using ut::operator""_test;

    "perft"_test = [] {
        constexpr int depth = 4;
        blocking_input input{"../epd/perft_long.txt"};
        std::vector<std::jthread> workers{};
        for (int i = 0; i < std::jthread::hardware_concurrency(); ++i) {
            workers.emplace_back([&input]() {
                char epd[256];
                while (input.read(epd)) {
                    std::cmatch match;
                    if (!std::regex_search(epd, match, epd_regex))
                        throw std::runtime_error("broken epd");
                    side_e side;
                    node node {match[1].str(), side};
                    size_t expected = std::atoll(match[1 + depth].str().data());

                    size_t count = side == WHITE ? perft<WHITE>(node, depth) : perft<BLACK>(node, depth);

                    // std::println("{}: {} {}", match[1].str(), count, expected);
                    ut::expect(count == expected) << match[1].str();
                }
            });
        }
    };
}