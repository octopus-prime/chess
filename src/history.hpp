#pragma once

#include "move.hpp"
#include "side.hpp"
#include <cstdint>
#include <type_traits>

class history_t {
    enum slot_e { ALL, GOOD };
    using entry_t = std::array<uint64_t, 2>;
    entry_t entries[SQUARE_MAX][SQUARE_MAX];

public:
    history_t() { clear(); }

    void clear() noexcept {
        for (auto &from : entries) {
            for (auto &to : from) {
                to.fill(0);
            }
        }
    }

    void put_good(const move_t &move) noexcept {
        entries[move.from()][move.to()][GOOD]++;
    }

    void put_all(const move_t &move) noexcept {
        entries[move.from()][move.to()][ALL]++;
    }

    int get(const move_t &move) const noexcept {
        const entry_t& entry = entries[move.from()][move.to()];
        return 100 * entry[GOOD] / (1 + entry[ALL]);
    }
};
