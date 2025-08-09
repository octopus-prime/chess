#pragma once

#include "position.hpp"
#include "transposition.hpp"
#include "killer.hpp"
#include "history.hpp"
#include "evaluator.hpp"
#include "move_picker.hpp"
#include <print>
#include <chrono>

struct searcher_t {

    struct statistics_t {
        size_t nodes = 0;
        // size_t qnodes = 0;
        size_t max_height = 0;
    };

    struct result_t {
        std::int32_t score;
        std::span<move_t> pv;

        result_t operator-() const noexcept {
            return {-score, pv};
        }
    };


    position_t& position;
    transposition_t& transposition;
    killer_t& killer;
    history_t& history;
    evaluator& evaluator;
    std::array<move_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;
    statistics_t stats;
    std::atomic<bool> should_stop_flag{false};

    void clear() noexcept {
        transposition.clear();
        killer.clear();
        history.clear();
        stats.nodes = 0;
        stats.max_height = 0;
        should_stop_flag = false;
    }

    void request_stop() noexcept {
        should_stop_flag = true;
    }

    bool should_stop() const noexcept {
        return should_stop_flag.load();
    }

    int operator()(int alpha, int beta, int height) noexcept {
        stats.nodes++;
        stats.max_height = std::max(stats.max_height, static_cast<size_t>(height));

        if (position.is_check()) {
            stats.nodes--;
            std::array<move_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;
            return (*this)(alpha, beta, height, 1, pv_buffer).score;
        }

        if (position.is_no_material()) {
            return 0;
        }

        side_e side = position.get_side();

        int stand_pat = position.get_material() + position.attackers(side).size() - position.attackers(~side).size();
        // int stand_pat = position.get_material() + evaluator.evaluate(position); // / 8;
        // int stand_pat = evaluator.evaluate(position) / 8;

        // // Prevent Q-search explosion
        // if (q_depth > 6) {
        //     return stand_pat;
        // }

        if (stand_pat >= beta) {
            return beta;
        }
        if (stand_pat > alpha) {
            alpha = stand_pat;
        }

        std::array<move_t, position_t::MAX_ACTIVE_MOVES_PER_PLY> buffer;
        std::span<move_t> moves = position.generate_active_moves(buffer);

        std::array<int, position_t::MAX_ACTIVE_MOVES_PER_PLY> gains;
        std::ranges::transform(moves, gains.begin(), [&](const move_t& move) {
            return position.see(move);
        });

        auto zip = std::views::zip(moves, gains);
        std::ranges::sort(zip, std::greater<>{}, [&](auto&& pair) {
            return std::get<1>(pair);
        });

        for (auto&& [move, gain] : zip) {
            if (gain < 0) {
                break;  // Skip moves that lose material
            }

            position.make_move(move);
            int score = -(*this)(-beta, -alpha, height + 1);
            position.undo_move(move);
            
            if (score >= beta) {
                return beta;
            }
            if (score > alpha) {
                alpha = score;
            }
        }
        
        return alpha;
    }

    result_t operator()(int alpha, int beta, int height, int depth, std::span<move_t, position_t::MAX_MOVES_PER_GAME> pv) noexcept {
        if (should_stop()) {
            return {alpha, {}};
        }

        stats.nodes++;
        stats.max_height = std::max(stats.max_height, static_cast<size_t>(height));

        if (position.is_no_material() || position.is_50_moves_rule() || position.is_3_fold_repetition()) {
            return {0, {}};
        }

        std::array<move_t, position_t::MAX_MOVES_PER_PLY> buffer;
        std::span<move_t> moves = position.generate_all_moves(buffer);

        if (moves.empty()) {
            return {position.is_check() ? -30000 + height : 0, {}};
        }

        if (depth == 0 && (position.is_check() || moves.size() == 1)) {
            depth++;
        }

        if (depth == 0) {
            stats.nodes--;
            int score = (*this)(alpha, beta, height);
            return {score, {}};
        }

        move_t best;
        const entry_t* const entry = transposition.get(position.hash());
        if (entry) {
            best = entry->move;
            if (entry->depth >= depth) {
                switch (entry->flag) {
                case flag_t::EXACT:
                    pv.front() = best;
                    return {entry->score, pv.first(1)};
                case flag_t::LOWER:
                    if (entry->score > alpha)
                        alpha = entry->score;
                    break;
                case flag_t::UPPER:
                    if (entry->score < beta)
                        beta = entry->score;
                    break;
                default:
                    break;
                }
                if (alpha >= beta) {
                    pv.front() = best;
                    return {beta, pv.first(1)};
                }
            }
        }

        std::array<move_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;

        const int R = depth / 4;
        if (depth > R + 2 && moves.size() > 8 && position.can_null_move()) {
            position.make_null_move();
            result_t result = -(*this)(-beta, -beta + 1, height + 1, depth - 1 - R, pv_buffer);
            position.undo_null_move();
            if (result.score >= beta) {
                result = (*this)(alpha, beta, height, depth - 2 - R, pv_buffer);
                if (result.score >= beta) {
                    return {beta, {}};
                }
            }
        }
        
        if (best == move_t{} && depth > 4) {
            auto pv = (*this)(alpha, beta, height, depth - 2, pv_buffer).pv;
            if (!pv.empty()) {
                best = pv.front();
            }
        }

        move_picker_t move_picker(position, history, killer.get(height), best, moves);

        size_t length = 0;
        bool pv_found = false;
        move_picker_t::phase_e pv_phase = move_picker_t::TT_MOVES;
        // bool check = !position.is_check();
        size_t index = 0;
        for (auto&& phase : move_picker_t::ALL) {
        for (auto&& [move, see_eval] : move_picker(phase)) {

            if (height == 0 && depth > 6) {
                std::println("info currmove {} currmovenumber {}", move, index + 1);
                std::fflush(stdout);
            }

            // if (depth == 1 && pv_found && !position.is_check() && position.get_material() + gain + 300 < alpha) {
            //     index++;
            //     continue;
            // }

            // if (!position.is_check() && pv_found && depth > 10 && index > 20 && gain <= 0) {
            //     index++;
            //     continue;
            // }


            position.make_move(move);
            result_t result;
            if (pv_found /* && !position.is_check()*/) {
                // if (depth > R + 2 && index > moves.size() / 2 && gain <= 0) {
                //     result = -(*this)(-alpha - 1, -alpha, height + 1, depth - 1 - R, pv_buffer);
                // } else {
                    result = -(*this)(-alpha - 1, -alpha, height + 1, depth - 1, pv_buffer);
                // }
                if (result.score >= alpha) {
                    result = -(*this)(-beta, -result.score, height + 1, depth - 1, pv_buffer);
                }
            } else {
                result = -(*this)(-beta, -alpha, height + 1, depth - 1, pv_buffer);
            }
            position.undo_move(move);
            if (phase == move_picker_t::QUIET_MOVES) {
                history.put_all(move);
            }

            index++;

            if (result.score >= beta) {
                transposition.put(position.hash(), move, beta, flag_t::LOWER, depth);
                if (phase == move_picker_t::QUIET_MOVES) {
                    killer.put(move, height);
                    history.put_good(move);
                }
                pv.front() = move;
                std::ranges::copy(result.pv, pv.begin() + 1);
                return {beta, pv.first(result.pv.size() + 1)};
            }

            if (result.score > alpha) {
                alpha = result.score;
                best = move;
                pv_found = true;
                pv_phase = phase;
                pv.front() = move;
                std::ranges::copy(result.pv, pv.begin() + 1);
                length = result.pv.size() + 1;
            }
        }
    }

        if (pv_found) {
            transposition.put(position.hash(), best, alpha, flag_t::EXACT, depth);
            if (pv_phase == move_picker_t::QUIET_MOVES) {
                killer.put(best, height);
                history.put_good(best);
            }
        } else {
            transposition.put(position.hash(), best, alpha, flag_t::UPPER, depth);
        }

        return {alpha, pv.first(length)};
    }

    move_t operator()(int depth) {
        using as_floating_point = std::chrono::duration<double, std::ratio<1>>;

        move_t best;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int iteration = 1; iteration <= depth; ++iteration) {
            result_t result = (*this)(-30000, 30000, 0, iteration, pv_buffer);
            if (should_stop()) {
                break;
            }
            best = result.pv.front();
            auto time1 = std::chrono::high_resolution_clock::now();
            auto time = duration_cast<as_floating_point>(time1 - t0).count();
            std::println("info depth {} seldepth {} score cp {} nodes {} nps {} hashfull {} time {} pv {}", iteration, stats.max_height, result.score, stats.nodes, size_t(stats.nodes / time), transposition.full(), size_t(time * 1000), result.pv);
            std::fflush(stdout);
            if (result.score < -29000 || result.score > 29000) {
                break;
            }
        }
        return best;
    }
};
