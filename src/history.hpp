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
    }

    void clear() noexcept {
        std::ranges::fill(butterfly_per_side_history, butterfly_entry_t{});
        std::ranges::fill(butterfly_low_ply_history, butterfly_entry_t{});
        std::ranges::fill(pawn_per_side_history, butterfly_entry_t{});
        std::ranges::fill(pawn_low_ply_history, butterfly_entry_t{});
        std::ranges::fill(piece_to_per_side_history, piece_to_entry_t{});
        std::ranges::fill(piece_to_low_ply_history, piece_to_entry_t{});
        std::ranges::fill(continuation_per_side_history, continuation_entry_t{});
        std::ranges::fill(continuation_low_ply_history, continuation_entry_t{});
    }

    void age() noexcept {
        constexpr uint16_t DECAY = 10;

        auto age_butterfly = [](butterfly_entry_t& entry) {
            for (auto &from : entry) {
                for (auto &to : from) {
                    to /= DECAY;
                }
            }
        };
        auto age_piece_to = [](piece_to_entry_t& entry) {
            for (auto &type : entry) {
                for (auto &to : type) {
                    to /= DECAY;
                }
            }
        };
        auto age_continuation = [](continuation_entry_t& entry) {
            for (auto &from_type : entry) {
                for (auto &from_square : from_type) {
                    for (auto &to_type : from_square) {
                        for (auto &to_square : to_type) {
                            to_square /= DECAY;
                        }
                    }
                }
            }
        };

        std::ranges::for_each(butterfly_per_side_history, age_butterfly);
        std::ranges::for_each(butterfly_low_ply_history, age_butterfly);
        std::ranges::for_each(pawn_per_side_history, age_butterfly);
        std::ranges::for_each(pawn_low_ply_history, age_butterfly);
        std::ranges::for_each(piece_to_per_side_history, age_piece_to);
        std::ranges::for_each(piece_to_low_ply_history, age_piece_to);
        std::ranges::for_each(continuation_per_side_history, age_continuation);
        std::ranges::for_each(continuation_low_ply_history, age_continuation);
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
        uint16_t clampedBonus = std::min<uint16_t>(bonus, factor);
        entry += clampedBonus - entry * clampedBonus / factor;
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
