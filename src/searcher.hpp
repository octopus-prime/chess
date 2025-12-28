#pragma once

#include "position.hpp"
#include "transposition.hpp"
#include "history.hpp"
#include "evaluator.hpp"
#include "move_picker.hpp"
#include <chrono>
#include <expected>

struct searcher_t {

    template <typename T>
    using qbuffer_t = std::array<T, position_t::MAX_ACTIVE_MOVES_PER_PLY>;

    template <typename T>
    using fbuffer_t = std::array<T, position_t::MAX_MOVES_PER_PLY>;

    struct statistics_t {
        size_t nodes = 0;
        uint16_t max_height = 0;
    };

    struct result_t {
        score_t score;
        std::span<move_t> pv;

        result_t operator-() const noexcept {
            return {static_cast<score_t>(-score), pv};
        }
    };


    position_t& position;
    transposition_t& transposition;
    history_t& history;
    evaluator& evaluator;
    std::function<bool()> should_stop;
    statistics_t stats;

    void clear() noexcept {
        transposition.clear();
        history.clear();
        stats.nodes = 0;
        stats.max_height = 0;
    }

    score_t qsearch(score_t alpha, score_t beta, uint16_t height) noexcept {
        stats.nodes++;
        stats.max_height = std::max(stats.max_height, height);

        if (position.is_no_material() || position.is_50_moves_rule() || position.is_3_fold_repetition())
            return DRAW;

        score_t stand_pat = evaluator.evaluate(position, alpha, beta);

        if (stand_pat >= beta)
            return beta;
        if (stand_pat > alpha)
            alpha = stand_pat;

        qbuffer_t<move_t> buffer;
        qbuffer_t<score_t> gains;

        auto moves = position.generate_active_moves(buffer);
        auto zip = std::views::zip(moves, gains);
        for (auto&& [move, gain] : zip) 
            gain = position.see(move);

        constexpr auto get_gain = [](auto&& t) -> score_t {
            return std::get<1>(t);
        };
        const auto is_gainful = [limit = std::max(0, alpha - stand_pat - 100)](score_t gain) -> bool {
            return gain >= limit;
        };

        auto tail = std::ranges::partition(zip, is_gainful, get_gain);
        auto todo = std::ranges::subrange(zip.begin(), tail.begin());
        std::ranges::sort(todo, std::greater<>{}, get_gain);

        for (move_t move : todo | std::views::keys) {
            position.make_move(move);
            score_t score = -qsearch(-beta, -alpha, height + 1);
            position.undo_move(move);
            
            if (score >= beta)
                return beta;
            if (score > alpha)
                alpha = score;
        }

        return alpha;
    }

    result_t fsearch(score_t alpha, score_t beta, uint16_t height, int16_t depth, std::span<move_t, position_t::MAX_MOVES_PER_GAME> pv) noexcept {
        if (should_stop()) {
            return {alpha, {}};
        }

        stats.nodes++;
        stats.max_height = std::max(stats.max_height, height);

        if (position.is_no_material() || position.is_50_moves_rule() || position.is_3_fold_repetition()) {
            return {DRAW, {}};
        }

        fbuffer_t<move_t> buffer;
        auto moves = position.generate_all_moves(buffer);

        if (moves.empty()) {
            return {position.is_check() ? static_cast<score_t>(MIN + height) : DRAW, {}};
        }

        bool pv_node = (beta - alpha > 1);

        if (pv_node && position.is_check()) {
            depth++;
        }

        if (depth <= 0) {
            stats.nodes--;
            score_t score = qsearch(alpha, beta, height);
            return {score, {}};
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
                    pv.front() = best;
                    return {beta, pv.first(1)};
                }
            }
        }

        // {
        //     score_t score = qsearch(alpha, beta, height);
        //     if (!position.is_check() && score < alpha - 10 - 150 * depth * depth) {
        //         return {score, {}};
        //     }
        //     if (!position.is_check() && score > beta + 50 + 150 * depth * depth) {
        //         return {static_cast<score_t>((2 * score + beta) / 3), {}};
        //     }
        // }

        // int old_alpha = alpha;

        std::array<move_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;

        if (depth > 2 && moves.size() > 8 && position.can_null_move()) {
            int16_t R = 2 + std::min(3, (depth - 1) / 3);
            position.make_null_move();
            result_t result = -fsearch(-beta, -beta + 1, height + 1, depth - 1 - R, pv_buffer);
            position.undo_null_move();
            if (result.score >= beta) {
                result = fsearch(beta - 1, beta, height, depth - 1 - R, pv_buffer);
                if (result.score >= beta) {
                    return {beta, {}};
                }
            }
        }

        // // ProbCut parameters (tune!)
        constexpr int16_t MIN_PROBCUT_DEPTH = 6;
        constexpr int16_t PROBCUT_REDUCTION = 3;
        if (depth >= MIN_PROBCUT_DEPTH && !position.is_check()) {

            score_t margin = 0 + 25 * (depth - MIN_PROBCUT_DEPTH);

            {
                score_t probcut = beta + margin;
                score_t score = qsearch(probcut, probcut + 1, height);
                if (score > probcut && depth - PROBCUT_REDUCTION > 0) {
                    score = fsearch(probcut, probcut + 1, height, depth - PROBCUT_REDUCTION, pv_buffer).score;
                }
                if (score > probcut) {
                    return {score, {}};
                }
            }

            {
                score_t probcut = alpha - margin;
                score_t score = qsearch(probcut - 1, probcut, height);
                if (score < probcut && depth - PROBCUT_REDUCTION > 0) {
                    score = fsearch(probcut - 1, probcut, height, depth - PROBCUT_REDUCTION, pv_buffer).score;
                    if (score < probcut) {
                        return {score, {}};
                    }
                }
            }
        }

        if (best == move_t{} && depth > 4) {
            auto pv = fsearch(alpha, beta, height, depth / 2, pv_buffer).pv;
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
                if (depth > 4 && !position_check && !move_check && eval.see <= 0 && eval.history == 0) {
                    result = -fsearch(-alpha - 1, -alpha, height + 1, depth / 2, pv_buffer);
                } else {
                    result = -fsearch(-alpha - 1, -alpha, height + 1, depth - 1, pv_buffer);
                }
                if (result.score >= alpha && result.score < beta) {
                    result = -fsearch(-beta, -alpha, height + 1, depth - 1, pv_buffer);
                }
            } else {
                result = -fsearch(-beta, -alpha, height + 1, depth - 1, pv_buffer);
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
                // history.put(best, height, depth);
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

    result_t asearch(score_t score, int16_t depth, std::span<move_t, position_t::MAX_MOVES_PER_GAME> pv) noexcept {
        constexpr score_t delta = 25;
        const score_t alpha = score - delta;
        const score_t beta = score + delta;
        result_t result = fsearch(alpha, beta, 0, depth, pv);
        if (should_stop()) {
            return result;
        }
        if (result.score <= alpha) {
            result = fsearch(MIN, beta, 0, depth, pv);
        } else if (result.score >= beta) {
            result = fsearch(alpha, MAX, 0, depth, pv);
        }
        return result;
    }

    enum unexpected_e : uint8_t { insufficient_material, rule50, repetition, checkmate, stalemate };

    std::expected<move_t, unexpected_e> isearch(int16_t depth) noexcept {
        using as_floating_point = std::chrono::duration<double, std::ratio<1>>;

        fbuffer_t<move_t> move_buffer;
        auto moves = position.generate_all_moves(move_buffer);

        if (moves.empty()) {
            if (position.is_check()) return std::unexpected(checkmate);
            else return std::unexpected(stalemate);
        }
        if (moves.size() == 1) {
            return moves.front();
        }

        move_t best{};
        std::array<move_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;
        auto t0 = Clock::now();
        score_t score = qsearch(MIN, MAX, 0);

        for (int16_t iteration = 1; iteration <= depth; ++iteration) {
            result_t result = asearch(score, iteration, pv_buffer);
            score = result.score;
            if (should_stop()) break;

            std::span<move_t> pv_for_print;
            if (!result.pv.empty()) {
                best = result.pv.front();
                pv_for_print = result.pv;
            } else {
                // Try TT move, else fallback to first legal move
                if (auto e = transposition.get(position.hash()); e && e->move != move_t{}) {
                    best = e->move;
                } else {
                    best = moves.front();
                }
                pv_buffer[0] = best;
                pv_for_print = std::span<move_t>{pv_buffer}.first(1);
            }

            auto t1 = Clock::now();
            auto time = duration_cast<as_floating_point>(t1 - t0).count();

            char buffer[1024];
            if (result.score < (MIN + 1000) || result.score > (MAX - 1000)) {
                int plies = MAX - std::abs(result.score);
                int mate_in = (plies + 1) / 2;
                if (result.score < DRAW) mate_in = -mate_in;
                char* out = std::format_to(buffer,
                    "info depth {} seldepth {} score mate {:+} nodes {} nps {} hashfull {} time {} pv {}\n",
                    iteration, stats.max_height, mate_in, stats.nodes, size_t(stats.nodes / time),
                    transposition.full(), size_t(time * 1000), pv_for_print);
                std::fwrite(buffer, sizeof(char), out - buffer, stdout);
            } else {
                char* out = std::format_to(buffer,
                    "info depth {} seldepth {} score cp {:+} nodes {} nps {} hashfull {} time {} pv {}\n",
                    iteration, stats.max_height, result.score, stats.nodes, size_t(stats.nodes / time),
                    transposition.full(), size_t(time * 1000), pv_for_print);
                std::fwrite(buffer, sizeof(char), out - buffer, stdout);
            }
            std::fflush(stdout);

            history.age();
        }

        return best;
    }

    std::expected<move_t, unexpected_e> operator()(int16_t depth) noexcept {
        constexpr static std::string_view unexpected_text[] = {
            "draw by insufficient material"sv,
            "draw by 50 moves rule"sv,
            "draw by 3-fold repetition"sv,
            "checkmate"sv,
            "stalemate"sv
        };
        constexpr std::string_view none = "bestmove (none)\n"sv;
        char buffer[100];
        auto result = isearch(depth);
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
