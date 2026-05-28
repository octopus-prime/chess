#pragma once

#include "move.hpp"
#include "position.hpp"
#include "history.hpp"

struct move_picker_t {

    enum phase_e {
        TT_MOVES,
        GOOD_CAPTURE_MOVES,
        KILLER_MOVES,
        QUIET_MOVES,
        BAD_CAPTURE_MOVES
    };

    struct eval_t {
        int16_t see;
        int32_t history;

        auto operator<=>(const eval_t& other) const noexcept = default;
    };

    constexpr static auto ALL = {TT_MOVES, GOOD_CAPTURE_MOVES, KILLER_MOVES, QUIET_MOVES, BAD_CAPTURE_MOVES};

    move_picker_t(position_t& position, history_t& history, move_t best, int height, std::span<move_t> moves,
                  std::array<move_t, 2> killers = {move_t{}, move_t{}}) noexcept
        : position{position}, history{history}, best{best}, height{height}, moves{moves}, killers{killers}, offset{0} {
    }

    auto operator()(phase_e phase) noexcept {
        auto remaining_zip = std::views::zip(
            moves.subspan(offset), 
            std::span{evals}.subspan(offset)
        );

        auto get_move = [](const auto& t) static -> move_t {
            return std::get<0>(t);
        };

        auto get_see = [](const auto& t) static -> int16_t {
            return std::get<1>(t).see;
        };

        auto get_history = [](const auto& t) static -> uint16_t {
            return std::get<1>(t).history;
        };

        auto eval_see = [&](move_t move) -> int16_t {
            return position.see(move);
        };

        auto eval_history = [&](move_t move) -> int32_t {
            return 100 * position.check(move) + history.get(move, height);
        };

        switch (phase) {
            case TT_MOVES: {
                auto tail = std::ranges::partition(remaining_zip, [&](move_t move) { return move == best; }, get_move);
                auto result = std::ranges::subrange(remaining_zip.begin(), tail.begin());
                offset += std::distance(remaining_zip.begin(), tail.begin());
                return result;
            }
            case GOOD_CAPTURE_MOVES: {
                for (auto&& [move, eval] : remaining_zip) { eval.see = eval_see(move); }
                auto tail = std::ranges::partition(remaining_zip, [](int16_t see) static { return see > 0; }, get_see);
                auto result = std::ranges::subrange(remaining_zip.begin(), tail.begin());
                std::ranges::sort(result, std::greater<>{}, get_see);
                offset += std::distance(remaining_zip.begin(), tail.begin());
                return result;
            }
            case KILLER_MOVES: {
                auto is_quiet_killer = [&](const auto& t) -> bool {
                    return get_see(t) == 0 && (get_move(t) == killers[0] || get_move(t) == killers[1]);
                };
                auto tail = std::ranges::partition(remaining_zip, is_quiet_killer);
                auto result = std::ranges::subrange(remaining_zip.begin(), tail.begin());
                offset += std::distance(remaining_zip.begin(), tail.begin());
                return result;
            }
            case QUIET_MOVES: {
                auto tail = std::ranges::partition(remaining_zip, [](int16_t see) static { return see == 0; }, get_see);
                auto result = std::ranges::subrange(remaining_zip.begin(), tail.begin());
                for (auto&& [move, eval] : result) { eval.history = eval_history(move); }
                std::ranges::sort(result, std::greater<>{}, get_history);
                offset += std::distance(remaining_zip.begin(), tail.begin());
                return result;
            }
            case BAD_CAPTURE_MOVES: {
                auto result = std::ranges::subrange(remaining_zip.begin(), remaining_zip.end());
                std::ranges::sort(result, std::greater<>{}, get_see);
                return result;
            }
        }
    }

private:
    position_t& position;
    history_t& history;
    move_t best;
    int height;
    std::span<move_t> moves;
    std::array<move_t, 2> killers;
    std::size_t offset;
    std::array<eval_t, position_t::MAX_MOVES_PER_PLY> evals;
};
