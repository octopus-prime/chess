#pragma once

#include "move.hpp"
#include "side.hpp"
#include <cstdint>
#include <type_traits>

class history_t {
  uint64_t count_good[2][64][64];
  // uint64_t count_all[2][64][64];

public:
  history_t() { clear(); }

  void clear() noexcept {
    for (int k = 0; k < 2; ++k) {
      for (int i = 0; i < 64; ++i) {
        for (int j = 0; j < 64; ++j) {
          count_good[k][i][j] = 0;
          // count_all[k][i][j] = 1;
        }
      }
    }
  }

  template <side_e side>
  void put_good(const move_t &move, int depth) noexcept {
    count_good[side][move.from()][move.to()] += 1ull << depth;
  }

  // template <side_e side>
  // void put_all(const move_t &move, int depth) noexcept {
  //   count_all[side][move.from()][move.to()] += 1ull << depth;
  // }

  template <side_e side> 
  uint64_t get(const move_t &move) const noexcept {
    return count_good[side][move.from()][move.to()];
    // return 100'000 * count_good[side][move.from()][move.to()] / count_all[side][move.from()][move.to()];
  }
};
