#pragma once

#include "position.hpp"
#include "transposition.hpp"
#include "history.hpp"
#include "evaluator2.hpp"
#include "uci.hpp"
#include <print>

struct searcher_t {

    struct statistics_t {
        size_t nodes = 0;
        // size_t qnodes = 0;
        size_t max_height = 0;
    };

    struct result_t {
        std::int32_t score;
        std::span<move2_t> pv;

        result_t operator-() const noexcept {
            return {-score, pv};
        }
    };


    position_t& position;
    transposition2_t& transposition;
    history2_t& history;
    evaluator2& evaluator;
    std::array<move2_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;
    statistics_t stats;
    std::atomic<bool> should_stop_flag{false};

    void clean() noexcept {
        transposition.clear();
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

        if (position.is_no_material()) {
            return 0;
        }

        side_e side = position.get_side();

        // int stand_pat = position.get_material() + position.attackers(side).size() - position.attackers(~side).size();
        int stand_pat = position.get_material() + evaluator.evaluate(position) / 16;
        // int stand_pat = evaluator.evaluate(position);

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

        std::array<move2_t, position_t::MAX_MOVES_PER_PLY> buffer;
        std::span<move2_t> moves = position.generate_moves(buffer, position.by(~side));

        std::array<int, position_t::MAX_MOVES_PER_PLY> gains;
        std::ranges::transform(moves, gains.begin(), [&](const move2_t& move) {
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

    result_t operator()(int alpha, int beta, int height, int depth, std::span<move2_t, position_t::MAX_MOVES_PER_GAME> pv) noexcept {
        if (should_stop()) {
            return {alpha, {}};
        }

        stats.nodes++;
        stats.max_height = std::max(stats.max_height, static_cast<size_t>(height));

        if (position.is_no_material() || position.is_50_moves_rule() || position.is_3_fold_repetition()) {
            return {0, {}};
        }

        std::array<move2_t, position_t::MAX_MOVES_PER_PLY> buffer;
        std::span<move2_t> moves = position.generate_moves(buffer, bitboards::ALL);

        if (moves.empty()) {
            return {position.is_check() ? -30000 + height : 0, {}};
        }

        if (position.is_check() || moves.size() == 1) {
            depth++;
        }

        if (depth == 0) {
            stats.nodes--;
            int score = (*this)(alpha, beta, height);
            return {score, {}};
        }

        move2_t best;
        const entry2_t* const entry = transposition.get(position.hash());
        if (entry) {
            best = entry->move;
            if (entry->depth >= depth) {
                switch (entry->flag) {
                case flag_t::EXACT:
                    pv[0] = best;
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
                    pv[0] = best;
                    return {beta, pv.first(1)};
                }
            }
        }

        std::array<move2_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;

        const int R = depth / 3;
        if (depth > R + 2 && position.can_null_move()) {
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

        std::array<int, position_t::MAX_MOVES_PER_PLY> gains;
        std::ranges::transform(moves, gains.begin(), [&](const move2_t& move) {
            return position.see(move) + history.get(move, height, position.get_side());
        });

        auto zip = std::views::zip(moves, gains);
        std::ranges::sort(zip, std::greater<>{}, [&](auto&& pair) {
            return std::get<0>(pair) == best ? INT_MAX : std::get<1>(pair);
        });

        size_t length = 0;
        bool pv_found = false;
        size_t index = 0;
        for (auto&& [move, gain] : zip) {

            if (height == 0 && depth > 6) {
                std::println("info currmove {} currmovenumber {}", move, index + 1);
                std::fflush(stdout);
            }

            // if (depth == 1 && pv_found && !position.is_check() && position.get_material() + gain + 100 < alpha) {
            //     break;  // pruning: skip moves that can't improve alpha
            // }

            if (!position.is_check() && pv_found && depth > 8 && index > 8 && gain <= 0) {
                index++;
                continue;
            }

            position.make_move(move);
            result_t result;
            if (pv_found) {
                // if (depth > 4 && index > 10) {
                //     result = -(*this)(-alpha - 1, -alpha, height + 1, depth - 1 - 2, pv_buffer);
                // } else {
                    result = -(*this)(-alpha - 1, -alpha, height + 1, depth - 1, pv_buffer);
                // }
                if (result.score >= alpha) {
                    result = -(*this)(-beta, -alpha, height + 1, depth - 1, pv_buffer);
                }
            } else {
                result = -(*this)(-beta, -alpha, height + 1, depth - 1, pv_buffer);
            }
            position.undo_move(move);
            history.put_all(move, height, position.get_side());

            index++;

            if (result.score >= beta) {
                transposition.put(position.hash(), move, beta, flag_t::LOWER, depth);
                history.put_good(move, height, position.get_side());
                pv.front() = move;
                std::ranges::copy(result.pv, pv.begin() + 1);
                return {beta, pv.first(result.pv.size() + 1)};
            }

            if (result.score > alpha) {
                alpha = result.score;
                best = move;
                pv_found = true;
                pv.front() = move;
                std::ranges::copy(result.pv, pv.begin() + 1);
                length = result.pv.size() + 1;
            }
        }

        if (pv_found) {
            transposition.put(position.hash(), best, alpha, flag_t::EXACT, depth);
            history.put_good(best, height, position.get_side());
        } else {
            transposition.put(position.hash(), best, alpha, flag_t::UPPER, depth);
        }

        return {alpha, pv.first(length)};
    }

    move2_t operator()(int depth) {
        using as_floating_point = std::chrono::duration<double, std::ratio<1>>;
        // std::println("\ndepth: {}, fen: {}", depth, fen);

        // position_t position{fen};
        // searcher_t search{position, transposition, history, evaluator};

        move2_t best;
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
            // std::println("Depth {}/{}: {} nodes, score {}, pv {}", d, search.stats.max_height, search.stats.nodes, score, pv);
            if (result.score < -29000 || result.score > 29000) {
                break;
            }
        }
        return best;
        // auto t1 = std::chrono::high_resolution_clock::now();
        // std::chrono::duration<double> elapsed = t1 - t0;

        // std::println("Total nodes: {}, elapsed time: {:.2f} seconds, performance: {:.2f} Mnps", 
        //     search.stats.nodes, 
        //     elapsed.count(), 
        //     (search.stats.nodes / elapsed.count()) / 1e6);

        // transposition.clear();
        // history.clear();
    }
};

struct foo_search {

    transposition2_t transposition;
    history2_t history;
    evaluator2 evaluator;
    std::array<move2_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;

    void operator()(std::string_view fen, int depth) {
        std::println("\ndepth: {}, fen: {}", depth, fen);

        position_t position{fen};
        searcher_t search{position, transposition, history, evaluator};

        auto t0 = std::chrono::high_resolution_clock::now();
        for (int d = 1; d <= depth; ++d) {
            auto [score, pv] = search(-30000, 30000, 0, d, pv_buffer);
            std::println("Depth {}/{}: {} nodes, score {}, pv {}", d, search.stats.max_height, search.stats.nodes, score, pv);
            if (score < -29000 || score > 29000) {
                break;
            }
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = t1 - t0;

        std::println("Total nodes: {}, elapsed time: {:.2f} seconds, performance: {:.2f} Mnps", 
            search.stats.nodes, 
            elapsed.count(), 
            (search.stats.nodes / elapsed.count()) / 1e6);

        transposition.clear();
        history.clear();
    }

};
