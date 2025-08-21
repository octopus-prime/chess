#pragma once

#include "move.hpp"
#include "side.hpp"
#include "position.hpp"
#include <cstdint>
#include <type_traits>
#include <algorithm>

struct history_t {

    history_t(position_t& position) : 
        position{position}, 
        butterfly_per_side_history(SIDE_MAX), 
        butterfly_low_ply_history(LOW_PLY), 
        pawn_per_side_history(SIDE_MAX), 
        pawn_low_ply_history(LOW_PLY), 
        piece_to_per_side_history(SIDE_MAX), 
        piece_to_low_ply_history(LOW_PLY), 
        continuation_per_side_history(SIDE_MAX), 
        continuation_low_ply_history(LOW_PLY) {
        clear();
    }

    void clear() noexcept {
        for (auto &from : butterfly_per_side_history) {
            for (auto &to : from) {
                std::ranges::fill(to, 0);
            }
        }
        for (auto &from : butterfly_low_ply_history) {
            for (auto &to : from) {
                std::ranges::fill(to, 0);
            }
        }
        for (auto &from : pawn_per_side_history) {
            for (auto &to : from) {
                std::ranges::fill(to, 0);
            }
        }
        for (auto &from : pawn_low_ply_history) {
            for (auto &to : from) {
                std::ranges::fill(to, 0);
            }
        }
        for (auto &piece : piece_to_per_side_history) {
            for (auto &side : piece) {
                std::ranges::fill(side, 0);
            }
        }
        for (auto &piece : piece_to_low_ply_history) {
            for (auto &side : piece) {
                std::ranges::fill(side, 0);
            }
        }
        for (auto &piece : continuation_per_side_history) {
            for (auto &from : piece) {
                for (auto &to : from) {
                    for (auto &to2 : to) {
                        std::ranges::fill(to2, 0);
                    }
                }
            }
        }
        for (auto &piece : continuation_low_ply_history) {
            for (auto &from : piece) {
                for (auto &to : from) {
                    for (auto &to2 : to) {
                        std::ranges::fill(to2, 0);
                    }
                }
            }
        }
    }

    void age() {
        constexpr int DECAY = 10;
        for (auto &from : butterfly_per_side_history) {
            for (auto &to : from) {
                for (auto &entry : to) {
                    entry /= DECAY;
                }
            }
        }
        for (auto &from : butterfly_low_ply_history) {
            for (auto &to : from) {
                for (auto &entry : to) {
                    entry /= DECAY;
                }
            }
        }
        for (auto &from : pawn_per_side_history) {
            for (auto &to : from) {
                for (auto &entry : to) {
                    entry /= DECAY;
                }
            }
        }
        for (auto &from : pawn_low_ply_history) {
            for (auto &to : from) {
                for (auto &entry : to) {
                    entry /= DECAY;
                }
            }
        }
        for (auto &piece : piece_to_per_side_history) {
            for (auto &side : piece) {
                for (auto &entry : side) {
                    entry /= DECAY;
                }
            }
        }
        for (auto &piece : piece_to_low_ply_history) {
            for (auto &side : piece) {
                for (auto &entry : side) {
                    entry /= DECAY;
                }
            }
        }
        for (auto &piece : continuation_per_side_history) {
            for (auto &from : piece) {
                for (auto &to : from) {
                    for (auto &to2 : to) {
                        for (auto &entry : to2) {
                            entry /= DECAY;
                        }
                    }
                }
            }
        }
        for (auto &piece : continuation_low_ply_history) {
            for (auto &from : piece) {
                for (auto &to : from) {
                    for (auto &to2 : to) {
                        for (auto &entry : to2) {
                            entry /= DECAY;
                        }
                    }
                }
            }
        }
    }

    void put(move_t move, int height, int16_t value) noexcept {
        move_t last_move = position.last_move();
        square_e from = move.from();
        square_e to = move.to();
        square_e last_to = last_move.to();
        type_e type = position.at(from).type();
        type_e last_type = position.at(last_to).type();
        side_e side = position.get_side();
        update(butterfly_per_side_history[side][from][to], value, 64000);
        update(piece_to_per_side_history[side][type][to], value, 64000);
        update(continuation_per_side_history[side][last_type][last_to][type][to], value, 64000);
        if (type == PAWN) {
            update(pawn_per_side_history[side][from][to], value, 64000);
        }
        if (height < LOW_PLY) {
            update(butterfly_low_ply_history[height][from][to], value, 64000);
            update(piece_to_low_ply_history[height][type][to], value, 64000);
            update(continuation_low_ply_history[height][last_type][last_to][type][to], value, 64000);
            if (type == PAWN) {
                update(pawn_low_ply_history[height][from][to], value, 64000);
            }
        }
    }

    uint16_t get(move_t move, int height) const noexcept {
        move_t last_move = position.last_move();
        square_e from = move.from();
        square_e to = move.to();
        square_e last_to = last_move.to();
        type_e type = position.at(from).type();
        type_e last_type = position.at(last_to).type();
        side_e side = position.get_side();
        uint32_t score = 0;
        score += butterfly_per_side_history[side][from][to];
        score += pawn_per_side_history[side][from][to];
        score += piece_to_per_side_history[side][type][to];
        score += continuation_per_side_history[side][last_type][last_to][type][to];
        if (height < LOW_PLY) {
            score += butterfly_low_ply_history[height][from][to];
            score += pawn_low_ply_history[height][from][to];
            score += piece_to_low_ply_history[height][type][to];
            score += continuation_low_ply_history[height][last_type][last_to][type][to];
        }
        return score / 16;
    }

    static void update(uint16_t& entry, uint16_t bonus, uint16_t factor) noexcept {
        uint16_t clampedBonus = std::clamp<uint16_t>(bonus, 0, +factor);
        entry += clampedBonus - entry * clampedBonus / factor;
        // entry += clampedBonus - entry * std::abs(clampedBonus) / factor;
    }

private:
    constexpr static inline size_t LOW_PLY = 4;

    using butterfly_entry_t = std::array<std::array<uint16_t, SQUARE_MAX>, SQUARE_MAX>;
    using piece_to_entry_t = std::array<std::array<uint16_t, SQUARE_MAX>, TYPE_MAX - 1>;
    using continuation_entry_t = std::array<std::array<std::array<std::array<uint16_t, SQUARE_MAX>, TYPE_MAX - 1>, SQUARE_MAX>, TYPE_MAX - 1>;

    position_t& position;
    std::vector<butterfly_entry_t> butterfly_per_side_history;
    std::vector<butterfly_entry_t> butterfly_low_ply_history;
    std::vector<butterfly_entry_t> pawn_per_side_history;
    std::vector<butterfly_entry_t> pawn_low_ply_history;
    std::vector<piece_to_entry_t> piece_to_per_side_history;
    std::vector<piece_to_entry_t> piece_to_low_ply_history;
    std::vector<continuation_entry_t> continuation_per_side_history;
    std::vector<continuation_entry_t> continuation_low_ply_history;
};
