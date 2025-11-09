#pragma once

#include "move.hpp"
#include "position.hpp"
#include "history.hpp"

struct move_picker_t {

    enum phase_e {
        TT_MOVES,
        GOOD_CAPTURE_MOVES,
        QUIET_MOVES,
        BAD_CAPTURE_MOVES
    };

    struct eval_t {
        int16_t see;
        uint16_t history;

        auto operator<=>(const eval_t& other) const noexcept = default;
    };

    constexpr static auto ALL = {TT_MOVES, GOOD_CAPTURE_MOVES, QUIET_MOVES, BAD_CAPTURE_MOVES};

    move_picker_t(position_t& position, history_t& history, move_t best, int height, std::span<move_t> moves) noexcept
        : position{position}, history{history}, best{best}, height{height}, moves{moves}, offset{0} {
    }

    auto operator()(phase_e phase) noexcept {
        auto remaining_zip = std::views::zip(
            moves.subspan(offset), 
            std::span{evals}.subspan(offset)
        );

        auto get_move = [](const auto& t) -> move_t {
            return std::get<0>(t);
        };

        auto get_eval = [](const auto& t) -> eval_t {
            return std::get<1>(t);
        };

        auto get_see = [](const auto& t) -> int16_t {
            return std::get<1>(t).see;
        };

        auto get_history = [](const auto& t) -> uint16_t {
            return std::get<1>(t).history;
        };

        auto eval_see = [&](move_t move) -> int16_t {
            return position.see(move);
            // return position.see(move) + 10 * position.check(move);
        };

        auto eval_history = [&](move_t move) -> uint16_t {
            return 16000 * position.check(move) + history.get(move, height);
            // return history.get(move, height);
        };

        switch (phase) {
            case TT_MOVES: {
                auto tail = std::ranges::partition(remaining_zip, [&](move_t move) { return move == best; }, get_move);
                auto result = std::ranges::subrange(remaining_zip.begin(), tail.begin());
                for (auto&& [move, eval] : result) { eval = {eval_see(move), eval_history(move)}; }
                offset += std::distance(remaining_zip.begin(), tail.begin());
                return result;
            }
            case GOOD_CAPTURE_MOVES: {
                for (auto&& [move, eval] : remaining_zip) { eval.see = eval_see(move); }
                auto tail = std::ranges::partition(remaining_zip, [](int16_t see) { return see > 0; }, get_see);
                auto result = std::ranges::subrange(remaining_zip.begin(), tail.begin());
                for (auto&& [move, eval] : result) { eval.history = eval_history(move); }
                std::ranges::sort(result, std::greater<>{}, get_eval);
                offset += std::distance(remaining_zip.begin(), tail.begin());
                return result;
            }
            case QUIET_MOVES: {
                auto tail = std::ranges::partition(remaining_zip, [](int16_t see) { return see == 0; }, get_see);
                auto result = std::ranges::subrange(remaining_zip.begin(), tail.begin());
                for (auto&& [move, eval] : result) { eval.history = eval_history(move); }
                std::ranges::sort(result, std::greater<>{}, get_history);
                offset += std::distance(remaining_zip.begin(), tail.begin());
                return result;
            }
            case BAD_CAPTURE_MOVES: {
                auto result = std::ranges::subrange(remaining_zip.begin(), remaining_zip.end());
                // for (auto&& [move, eval] : result) { eval.history = eval_history(move); }
                // std::ranges::sort(result, std::greater<>{}, get_eval);
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
    std::size_t offset;
    std::array<eval_t, position_t::MAX_MOVES_PER_PLY> evals;
};
