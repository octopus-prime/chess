#pragma once

#include "position.hpp"
#include "transposition.hpp"
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
    history_t& history;
    evaluator& evaluator;
    std::function<bool()> should_stop;
    std::array<move_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;
    statistics_t stats;
    // std::atomic<bool> should_stop_flag{false};

    void clear() noexcept {
        transposition.clear();
        history.clear();
        stats.nodes = 0;
        stats.max_height = 0;
        // should_stop_flag = false;
    }

    // void request_stop() noexcept {
    //     should_stop_flag = true;
    // }

    // bool should_stop() const noexcept {
    //     return should_stop_flag.load();
    // }

    int operator()(int alpha, int beta, int height) noexcept {
        stats.nodes++;
        stats.max_height = std::max(stats.max_height, static_cast<size_t>(height));

        // if (position.is_check()) {
        //     stats.nodes--;
        //     std::array<move_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;
        //     return (*this)(alpha, beta, height, 0, pv_buffer).score;
        // }

        // if (position.is_no_material()) {
        if (position.is_no_material() || position.is_50_moves_rule() || position.is_3_fold_repetition()) {
            return 0;
        }

        side_e side = position.get_side();

        // int stand_pat = position.get_material() + position.attackers(side).size() - position.attackers(~side).size();
        // int stand_pat = position.get_material() + evaluator.evaluate(position) / 8;
        // int stand_pat = evaluator.evaluate(position);
        int stand_pat = evaluator.evaluate(position, alpha, beta);
        // if (std::abs(stand_pat - alpha) < 50 || std::abs(stand_pat - beta) < 50) {
        //     stand_pat = evaluator.evaluate(position);
        // }

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

        std::array<int16_t, position_t::MAX_ACTIVE_MOVES_PER_PLY> gains;
        std::ranges::transform(moves, gains.begin(), [&](const move_t& move) {
            return position.see(move);
        });

        auto zip = std::views::zip(moves, gains);

        // auto tail = std::ranges::partition(zip, [&](const auto& pair) {
        //     return std::get<1>(pair) >= 0;  // Keep only non-negative gains
        // });

        // auto foo = std::ranges::subrange(zip.begin(), tail.begin());

        std::ranges::sort(zip, std::greater<>{}, [&](auto&& pair) {
            return std::get<1>(pair);
        });

        for (auto&& [move, gain] : zip) {
            if (gain < 0 && !position.check(move)) {
                continue;  // Skip moves that lose material
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

        // if (depth == 0 && (position.is_check()/* || moves.size() == 1 */)) {
        //     depth++;
        // }

        if (depth == 0) {
            stats.nodes--;
            int score = (*this)(alpha, beta, height);
            transposition.put(position.hash(), move_t{}, score, flag_t::EXACT, depth);
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

        int old_alpha = alpha;

        std::array<move_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;

        const int R = depth / 3;
        if (depth > 4 && moves.size() > 8 && position.can_null_move()) {
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

        move_picker_t move_picker{position, history, best, height, moves};
        size_t length = 0;
        bool pv_found = false;
        move_picker_t::phase_e pv_phase = move_picker_t::TT_MOVES;
        bool position_check = position.is_check();
        size_t index = 0;
        for (auto&& phase : move_picker_t::ALL) {
            size_t phase_index = 0;
            auto phase_moves = move_picker(phase);
        for (auto&& [move, eval] : phase_moves) {
            auto see_eval = eval.see;
            bool move_check = position.check(move);

            if (height == 0 && depth > 6) {
                // std::println("info currmove {} currmovenumber {} see {} hist {}", move, index + 1, eval.see, eval.history);
                std::println("info currmove {} currmovenumber {}", move, index + 1);
                std::fflush(stdout);
            }

            // if (depth <= 4 && pv_found && !position_check && !move_check && position.get_material() + see_eval + 200 * depth < alpha) {
            //     index++;
            //     phase_index++;
            //     continue;
            // }

            // if (depth == 1 && pv_found && !position_check && position.get_material() + see_eval + 150 < alpha) {
            //     index++;
            //     phase_index++;
            //     continue;
            // }

            // if (!position.is_check() && pv_found && depth > 10 && index > 20 && gain <= 0) {
            //     index++;
            //     continue;
            // }


            position.make_move(move);
            result_t result;
            if (pv_found) {
                // bool reduced = ((phase == move_picker_t::QUIET_MOVES && phase_index > phase_moves.size() / 3) || phase == move_picker_t::BAD_CAPTURE_MOVES);
                // if (depth > 4 && !position_check && !move_check && reduced) {
                //     result = -(*this)(-alpha - 1, -alpha, height + 1, depth - 1 - R, pv_buffer);
                // } else {
                    result = -(*this)(-alpha - 1, -alpha, height + 1, depth - 1, pv_buffer);
                // }
                if (result.score >= alpha && result.score < beta) {
                    result = -(*this)(-beta, -result.score, height + 1, depth - 1, pv_buffer);
                }
            } else {
                result = -(*this)(-beta, -alpha, height + 1, depth - 1, pv_buffer);
            }
            position.undo_move(move);
            index++;
            phase_index++;

            if (result.score >= beta) {
                transposition.put(position.hash(), move, beta, flag_t::LOWER, depth);
                history.put(move, height, 4 * depth);
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
                // history.put(best, height, depth * depth / 4);
            } else {
                // history.put(move, height, -depth * depth / 4);
            }
        }
    }

        if (pv_found) {
            transposition.put(position.hash(), best, alpha, flag_t::EXACT, depth);
            history.put(best, height, depth);
        } else {
            transposition.put(position.hash(), best, alpha, flag_t::UPPER, depth);
        }

        return {alpha, pv.first(length)};
    }

    move_t operator()(int depth) {
        using as_floating_point = std::chrono::duration<double, std::ratio<1>>;

        move_t best;
        auto t0 = Clock::now();
        for (int iteration = 1; iteration <= depth; ++iteration) {
            result_t result = (*this)(-30000, 30000, 0, iteration, pv_buffer);
            if (should_stop()) {
                break;
            }
            best = result.pv.front();
            auto t1 = Clock::now();
            auto time = duration_cast<as_floating_point>(t1 - t0).count();
            std::println("info depth {} seldepth {} score cp {} nodes {} nps {} hashfull {} time {} pv {}", iteration, stats.max_height, result.score, stats.nodes, size_t(stats.nodes / time), transposition.full(), size_t(time * 1000), result.pv);
            std::fflush(stdout);
            if (result.score < -29000 || result.score > 29000) {
                break;
            }
            history.age();
        }
        return best;
    }
};
