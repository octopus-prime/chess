#pragma once

#include "position.hpp"
#include "transposition.hpp"
#include "history.hpp"
#include "evaluator.hpp"
#include "correction.hpp"
#include "move_picker.hpp"
#include <chrono>
#include <cmath>
#include <expected>

struct searcher_t {

    enum search_e {PV, NON_PV};

    struct statistics_t {
        size_t nodes = 0;
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
    correction_t& correction;
    std::function<bool()> should_stop;
    statistics_t stats;

    void clear() noexcept {
        transposition.clear();
        history.clear();
        correction.clear();
        stats.nodes = 0;
        stats.max_height = 0;
    }

    template <search_e SearchType>
    int qsearch(int alpha, int beta, int height) noexcept {
        constexpr bool is_pv = SearchType == PV;

        stats.nodes++;
        stats.max_height = std::max(stats.max_height, static_cast<size_t>(height));

        if (position.is_no_material() || position.is_50_moves_rule() || position.is_3_fold_repetition())
            return 0;

        int stand_pat = evaluator.evaluate(position, alpha, beta);

        if (stand_pat >= beta)
            return beta;
        if (stand_pat > alpha)
            alpha = stand_pat;

        std::array<move_t, position_t::MAX_ACTIVE_MOVES_PER_PLY> buffer;
        std::span<move_t> moves = position.generate_active_moves(buffer);

        move_picker_t move_picker{position, history, move_t{}, height, moves};

        for (auto&& [move, gain] : move_picker(move_picker_t::GOOD_CAPTURE_MOVES)) {
            if (!is_pv && stand_pat + gain.see + 150 < alpha)
                break;

            position.make_move(move);
            int score = -qsearch<SearchType>(-beta, -alpha, height + 1);
            position.undo_move(move);
            
            if (score >= beta)
                return beta;
            if (score > alpha)
                alpha = score;
        }

        return alpha;
    }

    // // Add helper functions to detect and adjust mate scores
    static constexpr int MATE_SCORE = 30000;
    static constexpr int MATE_BOUND = 29000;

    template <search_e SearchType>
    result_t search(int alpha, int beta, int height, int depth, std::span<move_t, position_t::MAX_MOVES_PER_GAME> pv) noexcept {
        constexpr bool is_pv = SearchType == PV;

        stats.nodes++;
        stats.max_height = std::max(stats.max_height, static_cast<size_t>(height));

        if (should_stop())
            return {alpha, {}};

        if (position.is_no_material() || position.is_50_moves_rule() || position.is_3_fold_repetition())
            return {0, {}};

    // Mate distance pruning: don't search for mates we can't achieve
    {
        int mate_value_max = MATE_SCORE - height - 1;  // best mate we could give from here
        int mate_value_min = -MATE_SCORE + height + 1; // best mate we could receive from here
        
        if (alpha < mate_value_min) alpha = mate_value_min;
        if (beta > mate_value_max) beta = mate_value_max;
        if (alpha >= beta) return {alpha, {}};
    }

        move_t best;
        if (const auto entry = transposition.get(position.hash())) {
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
                    return {beta, {}};
                }
            }
        }

        if (/*depth == 0 &&*/ is_pv && position.is_check())
            depth++;

        if (depth == 0) {
            stats.nodes--;
            int score = qsearch<SearchType>(alpha, beta, height);
            return {score, {}};
        }

        int eval = evaluator.evaluate(position, alpha, beta);
        int static_eval = eval;  // raw — preserved for correction update
        {
            move_t lm = position.last_move();
            type_e lt = lm != move_t{} ? position.at(lm.to()).type() : NO_TYPE;
            int corr = correction.get(position.pawn_hash(), position.minor_hash(), position.major_hash(),
                                      position.get_side(), lt, lm.to());
            eval = std::clamp(eval + corr * 2, -29000, 29000);
        }

        // Razoring
        {
            auto margin = 550 + 350 * depth * depth;
            if (!is_pv && !position.is_check() && eval < alpha - margin && std::abs(alpha) < MATE_BOUND && std::abs(beta) < MATE_BOUND)
                return {qsearch<SearchType>(alpha, beta, height), {}};
        }

        // Futility pruning
        {
            auto margin = depth * (67 + 33 * (best != move_t{}));
            if (!is_pv && !position.is_check() && depth < 8 && eval - margin >= beta && std::abs(beta) < MATE_BOUND)
                return {(2 * beta + eval) / 3, {}};
        }

        
        std::array<move_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;

        if (!is_pv && depth > 3 && position.can_null_move() && std::abs(beta) < MATE_BOUND) {
            int R = 2 + std::log2f(depth + 1);
            position.make_null_move();
            result_t result = -search<NON_PV>(-beta, -beta + 1, height + 1, depth - 1 - R, pv_buffer);
            position.undo_null_move();
            if (result.score >= beta && std::abs(result.score) < MATE_BOUND) {
                result = search<SearchType>(alpha, beta, height, depth - 1 - R, pv_buffer);
                if (result.score >= beta && std::abs(result.score) < MATE_BOUND) {
                    return {beta, {}};
                }
            }
        }

        // // ProbCut: if a shallow search on high-SEE captures already exceeds a wider
        // // beta margin we can safely prune this node.
        // if (!is_pv && depth >= 5 && !position.is_check() && beta < MATE_BOUND && best == move_t{}) {
        //     int pc_beta = beta + 25 + depth * 25;
        //     std::array<move_t, position_t::MAX_ACTIVE_MOVES_PER_PLY> pc_buffer;
        //     std::span<move_t> pc_moves = position.generate_active_moves(pc_buffer);
        //     for (auto&& pc_move : pc_moves) {
        //         if (position.see(pc_move) < pc_beta - eval)
        //             continue;
        //         position.make_move(pc_move);
        //         result_t pc_result = -search<NON_PV>(-pc_beta, -pc_beta + 1, height + 1, std::max(1, depth - 4), pv_buffer);
        //         position.undo_move(pc_move);
        //         if (pc_result.score >= pc_beta)
        //             return {pc_beta, {}};
        //     }
        // }

        if (!is_pv && depth >= 7 && best == move_t{} && std::abs(alpha) < MATE_BOUND && std::abs(beta) < MATE_BOUND) 
            depth--;

        if (best == move_t{}  && depth > 5) {
            auto pv = search<PV>(alpha, beta, height, depth / 2, pv_buffer).pv;
            if (!pv.empty()) {
                best = pv.front();
            }
        }

        std::array<move_t, position_t::MAX_MOVES_PER_PLY> buffer;
        std::span<move_t> moves = position.generate_all_moves(buffer);

        if (moves.empty())
            return {position.is_check() ? -MATE_SCORE + height : 0, {}};

        // depth += !position.is_check() && moves.size() == 1; // extend if only one move

        move_picker_t move_picker{position, history, best, height, moves};
        size_t length = 0;
        size_t move_count = 0;
        bool pv_found = false;
        for (auto&& phase : move_picker_t::ALL) {
            for (auto&& [move, eval] : move_picker(phase)) {
                ++move_count;

                bool is_quiet = phase == move_picker_t::QUIET_MOVES || phase == move_picker_t::BAD_CAPTURE_MOVES;
                int lmr_depth = depth - 1;
                if (depth >= 3 && move_count > 2 && is_quiet && !position.is_check() && !position.check(move)) {
                    int R = int(std::logf(depth + 1) * std::logf(move_count + 1)) / 2;
                    lmr_depth = std::clamp(depth - 1 - R, depth / 2, depth - 1);
                }

                position.make_move(move);

                result_t result;
                if (!pv_found && lmr_depth == depth - 1) {
                    result = -search<SearchType>(-beta, -alpha, height + 1, depth - 1, pv_buffer);
                } else {
                    result = -search<NON_PV>(-alpha - 1, -alpha, height + 1, lmr_depth, pv_buffer);
                    if (result.score > alpha && is_pv) {
                        result = -search<SearchType>(-beta, -alpha, height + 1, depth - 1, pv_buffer);
                    }
                }

                position.undo_move(move);

                if (result.score >= beta) {

        if (is_pv && depth > 3) {
            move_t lm = position.last_move();
            type_e lt = lm != move_t{} ? position.at(lm.to()).type() : NO_TYPE;
            int bonus = (result.score - static_eval) * depth;
            correction.update(position.pawn_hash(), position.minor_hash(), position.major_hash(),
                              position.get_side(), lt, lm.to(), bonus);
        }


                    transposition.put(position.hash(), move, beta, flag_t::LOWER, depth);
                    history.put(move, height, 6 * depth);
                    pv.front() = move;
                    return {beta, pv.first(1)};
                }

                if (result.score > alpha) {
                    alpha = result.score;
                    best = move;
                    pv_found = true;
                    pv.front() = move;
                    std::ranges::copy(result.pv, pv.begin() + 1);
                    length = 1 + result.pv.size();
                }
            }
        }

        if (pv_found) {
            transposition.put(position.hash(), best, alpha, flag_t::EXACT, depth);
            history.put(best, height, depth);
        } else {
            transposition.put(position.hash(), best, alpha, flag_t::UPPER, depth);
        }

        if (is_pv && depth > 3) {
            move_t lm = position.last_move();
            type_e lt = lm != move_t{} ? position.at(lm.to()).type() : NO_TYPE;
            int bonus = (alpha - static_eval) * depth;
            correction.update(position.pawn_hash(), position.minor_hash(), position.major_hash(),
                              position.get_side(), lt, lm.to(), bonus);
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

    enum unexpected_e : uint8_t { insufficient_material, rule50, repetition, checkmate, stalemate };

    std::expected<move_t, unexpected_e> search(int depth) noexcept {
        using as_floating_point = std::chrono::duration<double, std::ratio<1>>;

        if (position.is_no_material()) {
            return std::unexpected(insufficient_material);
        }

        if (position.is_50_moves_rule()) {
            return std::unexpected(rule50);
        }

        if (position.is_3_fold_repetition()) {
            return std::unexpected(repetition);
        }

        std::array<move_t, position_t::MAX_MOVES_PER_PLY> move_buffer;
        std::span<move_t> moves = position.generate_all_moves(move_buffer);

        if (moves.empty()) {
            if (position.is_check()) {
                return std::unexpected(checkmate);
            } else {
                return std::unexpected(stalemate);
            }
        }

        if (moves.size() == 1) {
            return moves.front();
        }

        move_t best{};
        std::array<move_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;
        auto t0 = Clock::now();
        // int score = (*this)(-30000, +30000, 0);
        for (int iteration = 1; iteration <= depth; ++iteration) {
            result_t result = search<PV>(-30000, 30000, 0, iteration, pv_buffer);
            // result_t result = aspiration_window(score, iteration, pv_buffer);
            // score = result.score;
            if (should_stop()) {
                break;
            }
            best = result.pv.front();
            auto t1 = Clock::now();
            auto time = duration_cast<as_floating_point>(t1 - t0).count();

            char buffer[1024];

            if (result.score < -29000 || result.score > 29000) {
                constexpr int MATE_SCORE = 30000;
                int plies = MATE_SCORE - std::abs(result.score);
                int mate_in = (plies + 1) / 2;
                if (result.score < 0)
                    mate_in = -mate_in;
                char* out = std::format_to(buffer, "info depth {} seldepth {} score mate {:+} nodes {} nps {} hashfull {} time {} pv {}\n", 
                    iteration, stats.max_height, mate_in, stats.nodes, size_t(stats.nodes / time), transposition.full(), size_t(time * 1000), result.pv);
                std::fwrite(buffer, sizeof(char), out - buffer, stdout);
            } else {
                char* out = std::format_to(buffer, "info depth {} seldepth {} score cp {:+} nodes {} nps {} hashfull {} time {} pv {}\n",
                    iteration, stats.max_height, result.score, stats.nodes, size_t(stats.nodes / time), transposition.full(), size_t(time * 1000), result.pv);
                std::fwrite(buffer, sizeof(char), out - buffer, stdout);
            }

            std::fflush(stdout);

            history.age();
        }

        return best;
    }

    std::expected<move_t, unexpected_e> operator()(int depth) noexcept {
        constexpr static std::string_view unexpected_text[] = {
            "draw by insufficient material"sv,
            "draw by 50 moves rule"sv,
            "draw by 3-fold repetition"sv,
            "checkmate"sv,
            "stalemate"sv
        };
        constexpr std::string_view none = "bestmove (none)\n"sv;
        char buffer[100];
        auto result = search(depth);
        if (result) {
            char* out = std::format_to(buffer, "bestmove {}\n", result.value());
            std::fwrite(buffer, sizeof(char), out - buffer, stdout);
        } else {
            char* out = std::format_to(buffer, "info string {}\n", unexpected_text[result.error()]);
            std::fwrite(buffer, sizeof(char), out - buffer, stdout);
            std::fwrite(none.data(), sizeof(char), none.size(), stdout);
        }
        std::fflush(stdout);
        return result;
    }
};
