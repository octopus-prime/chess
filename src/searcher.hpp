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

    struct statistics_t {
        size_t nodes = 0;
        size_t max_height = 0;
        size_t next_test = 1000;
    };

    position_t& position;
    transposition_t& transposition;
    history_t& history;
    evaluator& evaluator;
    correction_t& correction;
    std::function<bool()> should_stop;
    statistics_t stats;
    std::array<std::array<move_t, 2>, position_t::MAX_MOVES_PER_GAME> killer_moves{};

    void clear() noexcept {
        transposition.clear();
        history.clear();
        correction.clear();
        for (auto& killers : killer_moves)
            killers.fill(move_t{});
        stats.nodes = 0;
        stats.max_height = 0;
        stats.next_test = 1000;
    }

    int operator()(int alpha, int beta, int height) noexcept {
        stats.nodes++;
        stats.max_height = std::max(stats.max_height, static_cast<size_t>(height));

        if (position.is_no_material())// || position.is_50_moves_rule() || position.is_3_fold_repetition())
            return 0;

        int stand_pat = evaluator.evaluate(position, alpha, beta);

        if (stand_pat >= beta)
            return beta;
        if (stand_pat > alpha)
            alpha = stand_pat;

        // {
        //     move_t lm = position.last_move();
        //     type_e lt = lm != move_t{} ? position.at(lm.to()).type() : NO_TYPE;
        //     stand_pat += correction.get(position.pawn_hash(), position.minor_hash(), position.major_hash(),
        //                                 position.get_side(), lt, lm.to());
        //     stand_pat = std::clamp(stand_pat, -29000, 29000);
        // }

        std::array<move_t, position_t::MAX_ACTIVE_MOVES_PER_PLY> buffer;
        std::span<move_t> moves = position.generate_active_moves(buffer);

        move_picker_t move_picker{position, history, move_t{}, height, moves};

        for (auto&& type : {move_picker_t::GOOD_CAPTURE_MOVES, move_picker_t::NEUTRAL_CAPTURE_MOVES}) {
        for (auto&& [move, gain] : move_picker(type)) {
            if (stand_pat + gain.see + 150 < alpha)
                break;

            position.make_move(move);
            int score = -(*this)(-beta, -alpha, height + 1);
            position.undo_move(move);
            
            if (score >= beta)
                return beta;
            if (score > alpha)
                alpha = score;
        }
        }

        return alpha;
    }

    int operator()(int alpha, int beta, int height, int depth) noexcept {
        stats.nodes++;
        stats.max_height = std::max(stats.max_height, static_cast<size_t>(height));

        if (stats.nodes > stats.next_test) {
            if (should_stop())
                return -32100;
            stats.next_test += 1000;
        }

        if (position.is_no_material() || position.is_50_moves_rule() || position.is_3_fold_repetition())
            return 0;

        bool is_pv = (beta - alpha) > 1;

        move_t best;
        if (const auto entry = transposition.get(position.hash())) {
            best = entry->move;
            if (entry->depth >= depth) {
                switch (entry->flag) {
                case flag_t::EXACT:
                    return entry->score;
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
                    return beta;
                }
            }
        }

        if (depth == 0 && position.is_check())
            depth++;

        if (depth == 0) {
            stats.nodes--;
            return (*this)(alpha, beta, height);
        }

        int eval = evaluator.evaluate(position, alpha, beta);
        int static_eval = eval;  // raw — preserved for correction update
        {
            move_t lm = position.last_move();
            type_e lt = lm != move_t{} ? position.at(lm.to()).type() : NO_TYPE;
            int corr = correction.get(position.pawn_hash(), position.minor_hash(), position.major_hash(),
                                      position.get_side(), lt, lm.to());
            eval = std::clamp(eval + 2 * corr, -29000, 29000);
        }

        // Futility pruning
        {
            auto margin = depth * (55 + 25 * (best != move_t{}));
            if (!is_pv && !position.is_check() && depth < 8 && eval - margin >= beta && beta > -29000 && eval < 29000)
                return (2 * beta + eval) / 3;
        }

        if (!is_pv && depth > 2 && position.can_null_move()) {
            int R = 2 + std::min(3, (depth - 1) / 3);
            position.make_null_move();
            int score = -(*this)(-beta, -beta + 1, height + 1, depth - 1 - R);
            if (score == 32100)
                return -32100;
            position.undo_null_move();
            if (score >= beta) {
                score = (*this)(alpha, beta, height, depth - 1 - R);
                if (score >= beta) {
                    return beta;
                }
            }
        }

        if (!is_pv && depth >= 7 && best == move_t{}) 
            depth--;

        if (best == move_t{}  && depth > 5) {
            (*this)(alpha, beta, height, depth / 2);
            if (const auto entry = transposition.get(position.hash())) {
                best = entry->move;
            }
        }

        std::array<move_t, position_t::MAX_MOVES_PER_PLY> buffer;
        std::span<move_t> moves = position.generate_all_moves(buffer);

        if (moves.empty())
            return position.is_check() ? -30000 + height : 0;

        auto killers = height >= 0 && static_cast<std::size_t>(height) < killer_moves.size()
            ? killer_moves[height]
            : std::array<move_t, 2>{};
        move_picker_t move_picker{position, history, best, height, moves, killers};
        size_t move_count = 0;
        bool pv_found = false;
        for (auto&& phase : move_picker_t::ALL) {
            for (auto&& [move, eval] : move_picker(phase)) {
                ++move_count;

                bool is_quiet = phase == move_picker_t::QUIET_MOVES || phase == move_picker_t::BAD_CAPTURE_MOVES;
                int lmr_depth = depth - 1;
                if (depth >= 3 && move_count > 2 && is_quiet && !position.is_check() && !position.check(move)) {
                    int R = std::max(1, (int)(std::logf(depth) * std::logf(move_count) * 0.67f + 0.33f));
                    R /= (1 + is_pv);
                    lmr_depth = std::clamp(depth - 1 - R, 1, depth - 1);
                }

                position.make_move(move);

                int score;
                if (!pv_found && lmr_depth == depth - 1) {
                    score = -(*this)(-beta, -alpha, height + 1, depth - 1);
                } else {
                    score = -(*this)(-alpha - 1, -alpha, height + 1, lmr_depth);
                    if (score > alpha) {
                        score = -(*this)(-beta, -alpha, height + 1, depth - 1);
                    }
                }
                position.undo_move(move);

                if (score == 32100)
                return -32100;

                if (score >= beta) {
                    transposition.put(position.hash(), move, beta, flag_t::LOWER, depth);
                    history.put(move, height, 8 * depth);
                    if (phase == move_picker_t::QUIET_MOVES && !position.is_exchange(move)) {
                        auto& killers = killer_moves[height];
                        if (killers[0] != move) {
                            killers[1] = killers[0];
                            killers[0] = move;
                        }
                    }
                    return beta;
                }

                if (score > alpha) {
                    alpha = score;
                    best = move;
                    pv_found = true;
                }
            }
        }

        if (pv_found) {
            transposition.put(position.hash(), best, alpha, flag_t::EXACT, depth);
            history.put(best, height, depth);
        } else {
            transposition.put(position.hash(), best, alpha, flag_t::UPPER, depth);
        }

        if (!position.is_check() && depth > 3) {
            move_t lm = position.last_move();
            type_e lt = lm != move_t{} ? position.at(lm.to()).type() : NO_TYPE;
            int bonus = (alpha - static_eval) * depth;
            correction.update(position.pawn_hash(), position.minor_hash(), position.major_hash(),
                              position.get_side(), lt, lm.to(), bonus);
        }

        return alpha;
    }

    std::span<move_t> extract_pv(std::span<move_t, position_t::MAX_MOVES_PER_GAME> buffer, int max_length) noexcept {
        size_t length = 0;
        while (static_cast<int>(length) < max_length) {
            const auto entry = transposition.get(position.hash());
            if (!entry || entry->move == move_t{})
                break;

            buffer[length++] = entry->move;
            position.make_move(entry->move);
        }

        for (size_t i = length; i-- > 0;)
            position.undo_move(buffer[i]);

        return buffer.first(length);
    }

    int aspiration_window(int eval, int depth) noexcept {
        const int delta = 25 + 2 * depth;
        int window_alpha = eval - delta;
        int window_beta = eval + delta;

        int score = (*this)(window_alpha, window_beta, 0, depth);
        if (score == 32100)
            return 32100;
        if (score <= window_alpha) {
            score = (*this)(-30000, window_beta, 0, depth);
        } else if (score >= window_beta) {
            score = (*this)(window_alpha, 30000, 0, depth);
        }
        return score;
    }

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
        int score = (*this)(-30000, +30000, 0);
        for (int iteration = 1; iteration <= depth; ++iteration) {
            score = aspiration_window(score, iteration);
            if (score == -32100)
                break;
            auto t1 = Clock::now();
            auto time = duration_cast<as_floating_point>(t1 - t0).count();
            std::span<move_t> pv = extract_pv(pv_buffer, iteration);
            best = pv.front();

            char buffer[1024];

            if (score < -29000 || score > 29000) {
                constexpr int MATE_SCORE = 30000;
                int plies = MATE_SCORE - std::abs(score);
                int mate_in = (plies + 1) / 2;
                if (score < 0)
                    mate_in = -mate_in;
                char* out = std::format_to(buffer, "info depth {} seldepth {} score mate {:+} nodes {} nps {} hashfull {} time {} pv {}\n", 
                    iteration, stats.max_height, mate_in, stats.nodes, size_t(stats.nodes / time), transposition.full(), size_t(time * 1000), pv);
                std::fwrite(buffer, sizeof(char), out - buffer, stdout);
            } else {
                char* out = std::format_to(buffer, "info depth {} seldepth {} score cp {:+} nodes {} nps {} hashfull {} time {} pv {}\n",
                    iteration, stats.max_height, score, stats.nodes, size_t(stats.nodes / time), transposition.full(), size_t(time * 1000), pv);
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
