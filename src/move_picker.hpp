#pragma once

#include "move.hpp"
#include "position.hpp"
#include "history.hpp"

struct move_picker_t {

    enum phase_e {
        TT_MOVES,
        GOOD_CAPTURE_MOVES,
        NEUTRAL_CAPTURE_MOVES,
        QUIET_MOVES,
        BAD_CAPTURE_MOVES
    };

    struct eval_t {
        int16_t see;
        uint16_t history;

        auto operator<=>(const eval_t& other) const noexcept = default;
    };

    struct entry_t {
        move_t move;
        eval_t eval;
    };

    constexpr static auto ALL = {TT_MOVES, GOOD_CAPTURE_MOVES, NEUTRAL_CAPTURE_MOVES, QUIET_MOVES, BAD_CAPTURE_MOVES};

    move_picker_t(position_t& position, history_t& history, move_t best, int height, std::span<move_t> input_moves) noexcept
        : position{position}, history{history}, best{best}, height{height}, input_moves{input_moves}, moves{entries.data(), input_moves.size()}, offset{0} {
    }

    auto operator()(phase_e phase) noexcept {
        auto remaining_moves = moves.subspan(offset);

        auto eval_see = [&](move_t move) -> int16_t {
            return position.see(move);
        };

        auto eval_history = [&](move_t move) -> uint16_t {
            return 16000 * position.check(move) + history.get(move, height);
        };

        switch (phase) {
            case TT_MOVES: {
                auto remaining_input_moves = input_moves.subspan(offset);
                auto tail = std::ranges::partition(remaining_input_moves, [&](move_t move) { return move == best; });
                auto count = std::distance(remaining_input_moves.begin(), tail.begin());
                for (std::size_t i = 0; i < count; ++i) {
                    remaining_moves[i] = {remaining_input_moves[i], {}};
                }
                auto result = std::ranges::subrange(remaining_moves.begin(), remaining_moves.begin() + count);
                offset += count;
                return result;
            }
            case GOOD_CAPTURE_MOVES: {
                auto remaining_input_moves = input_moves.subspan(offset);
                for (std::size_t i = 0; i < remaining_moves.size(); ++i) {
                    auto move = remaining_input_moves[i];
                    remaining_moves[i] = {move, {eval_see(move), 0}};
                }
                auto tail = std::ranges::partition(remaining_moves, [](const entry_t& entry) { return entry.eval.see > 0; });
                auto result = std::ranges::subrange(remaining_moves.begin(), tail.begin());
                std::ranges::sort(result, std::greater<>{}, [](const entry_t& entry) { return entry.eval.see; });
                offset += std::distance(remaining_moves.begin(), tail.begin());
                return result;
            }
            case NEUTRAL_CAPTURE_MOVES: {
                auto tail = std::ranges::partition(remaining_moves, [this](const entry_t& entry) { return entry.eval.see == 0 && position.is_exchange(entry.move); });
                auto result = std::ranges::subrange(remaining_moves.begin(), tail.begin());
                offset += std::distance(remaining_moves.begin(), tail.begin());
                return result;
            }
            case QUIET_MOVES: {
                auto tail = std::ranges::partition(remaining_moves, [](const entry_t& entry) { return entry.eval.see == 0; });// && !position.is_exchange(entry.move); });
                auto result = std::ranges::subrange(remaining_moves.begin(), tail.begin());
                for (auto& entry : result) { entry.eval.history = eval_history(entry.move); }
                std::ranges::sort(result, std::greater<>{}, [](const entry_t& entry) { return entry.eval.history; });
                offset += std::distance(remaining_moves.begin(), tail.begin());
                return result;
            }
            case BAD_CAPTURE_MOVES: {
                auto result = std::ranges::subrange(remaining_moves.begin(), remaining_moves.end());
                std::ranges::sort(result, std::greater<>{}, [](const entry_t& entry) { return entry.eval.see; });
                return result;
            }
        }
    }

private:
    position_t& position;
    history_t& history;
    move_t best;
    int height;
    std::span<move_t> input_moves;
    std::array<entry_t, position_t::MAX_MOVES_PER_PLY> entries;
    std::span<entry_t> moves;
    std::size_t offset;
};
