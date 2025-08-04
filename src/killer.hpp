#pragma once

#include "move.hpp"

struct killer_t {
    using entry_t = std::array<move_t, 2>;

    killer_t() noexcept : entries_(256) {
    }

    void clear() noexcept {
        std::ranges::fill(entries_, entry_t{});
    }

    void put(const move_t& move, std::size_t height) noexcept {
        auto& entry = entries_[height];
        if (entry[0] != move && entry[1] != move) {
            entry[1] = entry[0];
            entry[0] = move;
        }
    }

    const entry_t& get(std::size_t height) const noexcept {
        return entries_[height];
    }

private:
    std::vector<entry_t> entries_;
};
