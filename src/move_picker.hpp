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

        auto get_move = [](const auto& t) -> decltype(auto) {
            return std::get<0>(t);
        };

        auto get_eval = [](const auto& t) -> decltype(auto) {
            return std::get<1>(t);
        };

        auto get_see = [](const auto& t) -> decltype(auto) {
            return std::get<1>(t).see;
        };

        auto get_history = [](const auto& t) -> decltype(auto) {
            return std::get<1>(t).history;
        };

        // for (auto&& [move, eval] : remaining_zip) {
        //     eval = 100000 * position.see(move) + histories.get(move);
        // }

        constexpr uint16_t CHECK = 16000;

        switch (phase) {
            case TT_MOVES: {
                auto tail = std::ranges::partition(remaining_zip, [&](move_t move) { return move == best; }, get_move);
                auto result = std::ranges::subrange(remaining_zip.begin(), tail.begin());
                for (auto&& [move, eval] : result) { eval = {position.see(move), static_cast<uint16_t>(history.get(move, height) + CHECK * position.check(move))}; }
                offset += std::distance(remaining_zip.begin(), tail.begin());
                return result;
            }
            case GOOD_CAPTURE_MOVES: {
                // for (auto&& [move, eval] : remaining_zip) { eval = {position.see(move), history.get(move, height) + 10000 * position.check(move)}; }
                for (auto&& [move, eval] : remaining_zip) { eval.see = position.see(move); }
                auto tail = std::ranges::partition(remaining_zip, [](const int see) { return see > 0; }, get_see);
                auto result = std::ranges::subrange(remaining_zip.begin(), tail.begin());
                for (auto&& [move, eval] : result) { eval.history = history.get(move, height) + CHECK * position.check(move); }
                std::ranges::sort(result, std::greater<>{}, get_eval);
                offset += std::distance(remaining_zip.begin(), tail.begin());
                return result;
            }
            case QUIET_MOVES: {
                auto tail = std::ranges::partition(remaining_zip, [](int see) { return see == 0; }, get_see);
                auto result = std::ranges::subrange(remaining_zip.begin(), tail.begin());
                for (auto&& [move, eval] : result) { eval.history = history.get(move, height) + CHECK * position.check(move); }
                std::ranges::sort(result, std::greater<>{}, get_history);
                offset += std::distance(remaining_zip.begin(), tail.begin());
                return result;
            }
            case BAD_CAPTURE_MOVES: {
                auto result = std::ranges::subrange(remaining_zip.begin(), remaining_zip.end());
                for (auto&& [move, eval] : result) { eval.history = history.get(move, height) + CHECK * position.check(move); }
                std::ranges::sort(result, std::greater<>{}, get_eval);
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
