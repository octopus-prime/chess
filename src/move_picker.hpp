#pragma once

#include "move.hpp"
#include "position.hpp"

struct move_picker_t {

    enum phase_e {
        TT_MOVES,
        GOOD_CAPTURE_MOVES,
        QUIET_MOVES,
        BAD_CAPTURE_MOVES
    };

    constexpr static auto ALL = {TT_MOVES, GOOD_CAPTURE_MOVES, QUIET_MOVES, BAD_CAPTURE_MOVES};

    move_picker_t(position_t& position, history_t& history, const killer_t::entry_t& killers, move_t best, std::span<move_t> moves) noexcept
        : position{position}, history{history}, killers{killers}, best{best}, moves{moves}, current_offset{0} {
    }

    auto operator()(phase_e phase) noexcept {
        auto remaining_zip = std::views::zip(
            moves.subspan(current_offset), 
            std::span{see_evals}.subspan(current_offset)
        );
        
        switch (phase) {
            case TT_MOVES: {
                auto tail = std::ranges::partition(remaining_zip, [&](auto&& tuple) {
                    auto&& [move, see_eval] = tuple;
                    return move == best;
                });
                auto result = std::ranges::subrange(remaining_zip.begin(), tail.begin());
                current_offset += std::distance(remaining_zip.begin(), tail.begin());
                return result;
            }
            case GOOD_CAPTURE_MOVES: {
                for (auto&& [move, see_eval] : remaining_zip) {
                    see_eval = position.see(move);
                }
                auto tail = std::ranges::partition(remaining_zip, [&](auto&& tuple) {
                    auto&& [move, see_eval] = tuple;
                    return see_eval > 0;
                });
                auto result = std::ranges::subrange(remaining_zip.begin(), tail.begin());
                std::ranges::sort(result, std::greater<>{}, [&](auto&& tuple) {
                    auto&& [move, see_eval] = tuple;
                    return see_eval;
                });
                current_offset += std::distance(remaining_zip.begin(), tail.begin());
                return result;
            }
            case QUIET_MOVES: {
                auto tail = std::ranges::partition(remaining_zip, [&](auto&& tuple) {
                    auto&& [move, see_eval] = tuple;
                    return see_eval == 0;
                });
                auto result = std::ranges::subrange(remaining_zip.begin(), tail.begin());
                auto result_moves = result | std::views::elements<0>;

                std::array<int, position_t::MAX_MOVES_PER_PLY> history_evals;
                for (auto&& [move, history_eval] : std::views::zip(result_moves, history_evals)) {
                    history_eval = history.get(move);
                }
                std::ranges::sort(std::views::zip(result_moves, history_evals), std::greater<>{}, [&](auto&& tuple) {
                    auto&& [move, history_eval] = tuple;
                    return (move == killers[0] || move == killers[1]) ? INT_MAX : history_eval;
                });
                current_offset += std::distance(remaining_zip.begin(), tail.begin());
                return result;
            }
            case BAD_CAPTURE_MOVES: {
                auto result = std::ranges::subrange(remaining_zip.begin(), remaining_zip.end());
                std::ranges::sort(result, std::greater<>{}, [&](auto&& tuple) {
                    auto&& [move, see_eval] = tuple;
                    return see_eval;
                });
                return result;
            }
        }
    }

private:
    position_t& position;
    history_t& history;
    const killer_t::entry_t& killers;
    move_t best;
    std::span<move_t> moves;
    std::array<int, position_t::MAX_MOVES_PER_PLY> see_evals;
    size_t current_offset;
};
