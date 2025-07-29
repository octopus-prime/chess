#pragma once

#include "move.hpp"
#include "side.hpp"
#include <cstdint>
#include <type_traits>

class history_t {
  uint64_t count_good[2][64][64];
  uint64_t count_all[2][64][64];

public:
  history_t() { clear(); }

  void clear() noexcept {
    for (int k = 0; k < 2; ++k) {
      for (int i = 0; i < 64; ++i) {
        for (int j = 0; j < 64; ++j) {
          count_good[k][i][j] = 0;
          count_all[k][i][j] = 1;
        }
      }
    }
  }

  template <side_e side>
  void put_good(const move_t &move, int depth) noexcept {
    count_good[side][move.from()][move.to()] += 1ull << depth;
  }

  void put_good(const move2_t &move, int depth, side_e side) noexcept {
    count_good[side][move.from()][move.to()] += 1ull << depth;
  }

  // template <side_e side>
  // void put_all(const move_t &move, int depth) noexcept {
  //   count_all[side][move.from()][move.to()] += 1ull << depth;
  // }

  void put_all(const move2_t &move, int depth, side_e side) noexcept {
    count_all[side][move.from()][move.to()] += 1ull << depth;
  }

  template <side_e side> 
  uint64_t get(const move_t &move) const noexcept {
    return count_good[side][move.from()][move.to()];
    // return 100'000 * count_good[side][move.from()][move.to()] / count_all[side][move.from()][move.to()];
  }

    uint64_t get(const move2_t &move, side_e side) const noexcept {
      // return count_good[side][move.from()][move.to()];
    return 100 * count_good[side][move.from()][move.to()] / count_all[side][move.from()][move.to()];
  }

};

class history2_t {
  enum slot_t { ALL, GOOD };
  using bucket_t = std::array<std::array<std::array<std::array<uint64_t, 2>, SQUARE_MAX>, SQUARE_MAX>, SIDE_MAX>;
  std::vector<bucket_t> buckets;

public:
  history2_t() : buckets(1024) { clear(); }

  void clear() noexcept {
    for (auto &bucket : buckets) {
      for (auto &side : bucket) {
        for (auto &from : side) {
          from.fill({0, 0});
        }
      }
    }
  }

  void put_good(const move2_t &move, int height, side_e side) noexcept {
    buckets[height][side][move.from()][move.to()][GOOD] ++;
  }

  void put_all(const move2_t &move, int height, side_e side) noexcept {
    buckets[height][side][move.from()][move.to()][ALL] ++;
  }

  int get(const move2_t &move, int height, side_e side) const noexcept {
    const auto& slot = buckets[height][side][move.from()][move.to()];
    return 100 * slot[GOOD] / (1 + slot[ALL]);
  }

};
