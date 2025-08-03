#pragma once

#include "move.hpp"
#include "side.hpp"
#include <cstdint>
#include <type_traits>

class history_t {
  enum slot_t { ALL, GOOD };
  using bucket_t = std::array<std::array<std::array<std::array<uint64_t, 2>, SQUARE_MAX>, SQUARE_MAX>, SIDE_MAX>;
  std::vector<bucket_t> buckets;

public:
  history_t() : buckets(256) { clear(); }

  void clear() noexcept {
    for (auto &bucket : buckets) {
      for (auto &side : bucket) {
        for (auto &from : side) {
          from.fill({0, 0});
        }
      }
    }
  }

  void put_good(const move_t &move, int height, side_e side) noexcept {
    buckets[height][side][move.from()][move.to()][GOOD] ++;
  }

  void put_all(const move_t &move, int height, side_e side) noexcept {
    buckets[height][side][move.from()][move.to()][ALL] ++;
  }

  int get(const move_t &move, int height, side_e side) const noexcept {
    const auto& slot = buckets[height][side][move.from()][move.to()];
    return 100 * slot[GOOD] / (1 + slot[ALL]);
  }

};
