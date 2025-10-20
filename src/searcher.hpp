#pragma once

#include "position.hpp"
#include "transposition.hpp"
#include "history.hpp"
#include "evaluator.hpp"
#include "move_picker.hpp"
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

        // side_e side = position.get_side();

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

        // bool pos_check = position.is_check();

        for (auto&& [move, gain] : zip) {
            // bool move_check = position.check(move);

            if (gain < 0 || stand_pat + gain + 100 < alpha) {
                continue;
            }

            // if (!pos_check && !move_check && (gain < 0 || stand_pat + gain + 100 < alpha)) {
            //     continue;
            // }

            // if (!pos_check && !move_check && gain < 0) {
            //     continue;
            // }

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

        // int old_alpha = alpha;

        std::array<move_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;

        const int R = 1 + depth / 3;
        if (depth > 4 && moves.size() > 8 && position.can_null_move()) {
            position.make_null_move();
            result_t result = -(*this)(-beta, -beta + 1, height + 1, depth - 1 - R, pv_buffer);
            position.undo_null_move();
            if (result.score >= beta) {
                result = (*this)(alpha, beta, height, depth - 1 - R, pv_buffer);
                if (result.score >= beta) {
                    return {beta, {}};
                }
            }
        }
        
        if (best == move_t{} && depth > 4) {
            auto pv = (*this)(alpha, beta, height, depth - R, pv_buffer).pv;
            if (!pv.empty()) {
                best = pv.front();
            }
        }

        move_picker_t move_picker{position, history, best, height, moves};
        size_t length = 0;
        bool pv_found = false;
        // move_picker_t::phase_e pv_phase = move_picker_t::TT_MOVES;
        bool position_check = position.is_check();
        // size_t index = 0;
        for (auto&& phase : move_picker_t::ALL) {
        //     size_t phase_index = 0;
            // auto phase_moves = move_picker(phase);
        for (auto&& [move, eval] : move_picker(phase)) {
            // auto see_eval = eval.see;
            // bool move_check = position.check(move);

            // if (height == 0 && depth > 6) {
            //     // std::println("info currmove {} currmovenumber {} see {} hist {}", move, index + 1, eval.see, eval.history);
            //     std::println("info currmove {} currmovenumber {}", move, index + 1);
            //     std::fflush(stdout);
            // }

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
            bool move_check = position.is_check();
            result_t result;
            if (pv_found) {
                // bool reduced = (phase == move_picker_t::QUIET_MOVES || phase == move_picker_t::BAD_CAPTURE_MOVES) && eval.history == 0;
                bool reduced = eval.see <= 0 && eval.history == 0;
                if (depth > 4 && !position_check && !move_check && reduced) {
                    result = -(*this)(-alpha - 1, -alpha, height + 1, depth - 1 - R, pv_buffer);
                } else {
                    result = -(*this)(-alpha - 1, -alpha, height + 1, depth - 1, pv_buffer);
                }
                if (result.score >= alpha && result.score < beta) {
                    result = -(*this)(-beta, -result.score, height + 1, depth - 1, pv_buffer);
                }
            } else {
                result = -(*this)(-beta, -alpha, height + 1, depth - 1, pv_buffer);
            }
            position.undo_move(move);
            // index++;
            // phase_index++;

            if (result.score >= beta) {
                transposition.put(position.hash(), move, beta, flag_t::LOWER, depth);
                history.put(move, height, 6 * depth);
                pv.front() = move;
                std::ranges::copy(result.pv, pv.begin() + 1);
                return {beta, pv.first(result.pv.size() + 1)};
            }

            if (result.score > alpha) {
                alpha = result.score;
                best = move;
                pv_found = true;
                // pv_phase = phase;
                pv.front() = move;
                std::ranges::copy(result.pv, pv.begin() + 1);
                length = result.pv.size() + 1;
                history.put(best, height, depth);
            } else {
                // history.put(move, height, -depth);
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

    // result_t aspiration_window(int score, int depth, std::span<move_t, position_t::MAX_MOVES_PER_GAME> pv) noexcept {
    //     const int ASPIRATION_DELTA = 50;
    //     int window_alpha = score - ASPIRATION_DELTA;
    //     int window_beta = score + ASPIRATION_DELTA;

    //     result_t result = (*this)(window_alpha, window_beta, 0, depth, pv);
    //     if (result.score <= window_alpha || result.score >= window_beta) {
    //         result = (*this)(-30000, window_beta, 0, depth, pv);
    //     } else if (result.score >= window_beta) {
    //         result = (*this)(window_alpha, 30000, 0, depth, pv);
    //     }
    //     return result;
    // }

    // result_t aspiration_window(int score, int depth, std::span<move_t, position_t::MAX_MOVES_PER_GAME> pv) noexcept {
    //     constexpr int MATE = 30000;
    //     int delta = 25; // initial half-window; tune if desired (e.g., 16 + 4*depth)

    //     int alpha = std::max(-MATE, score - delta);
    //     int beta  = std::min(+MATE, score + delta);

    //     result_t result = (*this)(alpha, beta, 0, depth, pv);

    //     std::array<move_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;
    //     while (result.score <= alpha || result.score >= beta) {
    //         if (should_stop()) break;
    //         delta <<= 2;
    //         if (result.score <= alpha)
    //             alpha = std::max(-MATE, score - delta);
    //         else
    //             beta = std::min(+MATE, score + delta);
    //         result_t result2 = (*this)(alpha, beta, 0, depth, pv_buffer);
    //         if (!result2.pv.empty()) {
    //             result = result_t{result2.score, pv.first(result2.pv.size())};
    //             std::ranges::copy(result2.pv, pv.begin());
    //         } else {
    //             result = {result2.score, result.pv};
    //         }
    //         if (alpha <= -MATE || beta >= MATE)
    //             break; // full window reached
    //     }

    //     return result;
    // }

    move_t operator()(int depth) {
        using as_floating_point = std::chrono::duration<double, std::ratio<1>>;

        constexpr std::string_view none = "bestmove (none)\n"sv;

        if (position.is_no_material()) {
            constexpr std::string_view info = "info string draw by insufficient material\n"sv;
            std::fwrite(info.data(), sizeof(char), info.size(), stdout);
            std::fwrite(none.data(), sizeof(char), none.size(), stdout);
            std::fflush(stdout);
            return move_t{};
        }

        if (position.is_50_moves_rule()) {
            constexpr std::string_view info = "info string draw by 50 moves rule\n"sv;
            std::fwrite(info.data(), sizeof(char), info.size(), stdout);
            std::fwrite(none.data(), sizeof(char), none.size(), stdout);
            std::fflush(stdout);
            return move_t{};
        }

        if (position.is_3_fold_repetition()) {
            constexpr std::string_view info = "info string draw by 3-fold repetition\n"sv;
            std::fwrite(info.data(), sizeof(char), info.size(), stdout);
            std::fwrite(none.data(), sizeof(char), none.size(), stdout);
            std::fflush(stdout);
            return move_t{};
        }

        std::array<move_t, position_t::MAX_MOVES_PER_PLY> move_buffer;
        std::span<move_t> moves = position.generate_all_moves(move_buffer);

        if (moves.empty()) {
            if (position.is_check()) {
                constexpr std::string_view info = "info string checkmate\n"sv;
                std::fwrite(info.data(), sizeof(char), info.size(), stdout);
                std::fwrite(none.data(), sizeof(char), none.size(), stdout);
                std::fflush(stdout);
            } else {
                constexpr std::string_view info = "info string stalemate\n"sv;
                std::fwrite(info.data(), sizeof(char), info.size(), stdout);
                std::fwrite(none.data(), sizeof(char), none.size(), stdout);
                std::fflush(stdout);
            }
            return move_t{};
        }

        if (moves.size() == 1) {
            // constexpr std::string_view info = "info string singular move\n"sv;
            char buffer[20];
            char* out = std::format_to(buffer, "bestmove {}\n", moves.front());
            // std::fwrite(info.data(), sizeof(char), info.size(), stdout);
            std::fwrite(buffer, sizeof(char), out - buffer, stdout);
            std::fflush(stdout);
            return moves.front();
        }

        move_t best{};
        std::array<move_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;
        auto t0 = Clock::now();
        // int score = (*this)(-30000, +30000, 0);
        for (int iteration = 1; iteration <= depth; ++iteration) {
            result_t result = (*this)(-30000, 30000, 0, iteration, pv_buffer);
            // result_t result = aspiration_window(score, iteration, pv_buffer);
            // score = result.score;
            if (should_stop()) {
                break;
            }
            best = result.pv.front();
            auto t1 = Clock::now();
            auto time = duration_cast<as_floating_point>(t1 - t0).count();

            if (result.score < -29000 || result.score > 29000) {
                constexpr int MATE_SCORE = 30000;
                int plies = MATE_SCORE - std::abs(result.score);
                int mate_in = (plies + 1) / 2;
                if (result.score < 0)
                    mate_in = -mate_in;
                char buffer[1024];
                char* out = std::format_to(buffer, "info depth {} seldepth {} score mate {:+} nodes {} nps {} hashfull {} time {} pv {}\n", 
                    iteration, stats.max_height, mate_in, stats.nodes, size_t(stats.nodes / time), transposition.full(), size_t(time * 1000), result.pv);
                std::fwrite(buffer, sizeof(char), out - buffer, stdout);
                std::fflush(stdout);
            } else {
                char buffer[1024];
                char* out = std::format_to(buffer, "info depth {} seldepth {} score cp {:+} nodes {} nps {} hashfull {} time {} pv {}\n",
                    iteration, stats.max_height, result.score, stats.nodes, size_t(stats.nodes / time), transposition.full(), size_t(time * 1000), result.pv);
                std::fwrite(buffer, sizeof(char), out - buffer, stdout);
                std::fflush(stdout);
            }

            history.age();
        }

        char buffer[20];
        char* out = std::format_to(buffer, "bestmove {}\n", best);
        std::fwrite(buffer, sizeof(char), out - buffer, stdout);
        std::fflush(stdout);
        return best;
    }
};
