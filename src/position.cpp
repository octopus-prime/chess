// #include "position.hpp"
// #include "transposition.hpp"
// #include "history.hpp"
// #include "evaluator2.hpp"
// #include "searcher.hpp"
#include "uci.hpp"
#include <print>

struct perft_t {
    position_t& position;

    size_t operator()(int depth) noexcept {
        std::array<move2_t, position_t::MAX_MOVES_PER_PLY> buffer;
        std::span<move2_t> moves = position.generate_moves(buffer, bitboards::ALL);

        if (depth == 0) {
            return 1;
        }

        if (depth == 1) {
            return moves.size();
        }

        return std::ranges::fold_left(moves, size_t{}, [&](size_t acc, const move2_t& move) {
            position.make_move(move);
            acc += (*this)(depth - 1);
            position.undo_move(move);
            return acc; 
        });
    }
};

// struct searcher_t {

//     struct statistics_t {
//         size_t nodes = 0;
//         // size_t qnodes = 0;
//         size_t max_height = 0;
//     };

//     struct result_t {
//         std::int32_t score;
//         std::span<move2_t> pv;

//         result_t operator-() const noexcept {
//             return {-score, pv};
//         }
//     };


//     position_t& position;
//     transposition2_t& transposition;
//     history2_t& history;
//     evaluator2& evaluator;
//     statistics_t stats;

//     int operator()(int alpha, int beta, int height) noexcept {
//         stats.nodes++;
//         stats.max_height = std::max(stats.max_height, static_cast<size_t>(height));

//         if (position.is_no_material()) {
//             return 0;
//         }

//         side_e side = position.get_side();

//         // int stand_pat = position.get_material() + position.attackers(side).size() - position.attackers(~side).size();
//         int stand_pat = position.get_material() + evaluator.evaluate(position) / 16;
//         // int stand_pat = evaluator.evaluate(position);

//         // // Prevent Q-search explosion
//         // if (q_depth > 6) {
//         //     return stand_pat;
//         // }

//         if (stand_pat >= beta) {
//             return beta;
//         }
//         if (stand_pat > alpha) {
//             alpha = stand_pat;
//         }

//         std::array<move2_t, position_t::MAX_MOVES_PER_PLY> buffer;
//         std::span<move2_t> moves = position.generate_moves(buffer, position.by(~side));

//         std::array<int, position_t::MAX_MOVES_PER_PLY> gains;
//         std::ranges::transform(moves, gains.begin(), [&](const move2_t& move) {
//             return position.see(move);
//         });

//         auto zip = std::views::zip(moves, gains);
//         std::ranges::sort(zip, std::greater<>{}, [&](auto&& pair) {
//             return std::get<1>(pair);
//         });

//         for (auto&& [move, gain] : zip) {
//             if (gain < 0) {
//                 break;  // Skip moves that lose material
//             }

//             position.make_move(move);
//             int score = -(*this)(-beta, -alpha, height + 1);
//             position.undo_move(move);
            
//             if (score >= beta) {
//                 return beta;
//             }
//             if (score > alpha) {
//                 alpha = score;
//             }
//         }
        
//         return alpha;
//     }

//     result_t operator()(int alpha, int beta, int height, int depth, std::span<move2_t, position_t::MAX_MOVES_PER_GAME> pv) noexcept {
//         stats.nodes++;
//         stats.max_height = std::max(stats.max_height, static_cast<size_t>(height));

//         if (position.is_no_material() || position.is_50_moves_rule() || position.is_3_fold_repetition()) {
//             return {0, {}};
//         }

//         std::array<move2_t, position_t::MAX_MOVES_PER_PLY> buffer;
//         std::span<move2_t> moves = position.generate_moves(buffer, bitboards::ALL);

//         if (moves.empty()) {
//             return {position.is_check() ? -30000 + height : 0, {}};
//         }

//         if (position.is_check() || moves.size() == 1) {
//             depth++;
//         }

//         if (depth == 0) {
//             stats.nodes--;
//             int score = (*this)(alpha, beta, height);
//             return {score, {}};
//         }

//         move2_t best;
//         const entry2_t* const entry = transposition.get(position.hash());
//         if (entry) {
//             best = entry->move;
//             if (entry->depth >= depth) {
//                 switch (entry->flag) {
//                 case flag_t::EXACT:
//                     pv[0] = best;
//                     return {entry->score, pv.first(1)};
//                 case flag_t::LOWER:
//                     if (entry->score > alpha)
//                         alpha = entry->score;
//                     break;
//                 case flag_t::UPPER:
//                     if (entry->score < beta)
//                         beta = entry->score;
//                     break;
//                 default:
//                     break;
//                 }
//                 if (alpha >= beta) {
//                     pv[0] = best;
//                     return {beta, pv.first(1)};
//                 }
//             }
//         }

//         std::array<move2_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;

//         const int R = depth / 3;
//         if (depth > R + 2 && position.can_null_move()) {
//             position.make_null_move();
//             result_t result = -(*this)(-beta, -beta + 1, height + 1, depth - 1 - R, pv_buffer);
//             position.undo_null_move();
//             if (result.score >= beta) {
//                 result = (*this)(alpha, beta, height, depth - 2 - R, pv_buffer);
//                 if (result.score >= beta) {
//                     return {beta, {}};
//                 }
//             }
//         }

//         std::array<int, position_t::MAX_MOVES_PER_PLY> gains;
//         std::ranges::transform(moves, gains.begin(), [&](const move2_t& move) {
//             return position.see(move) + history.get(move, height, position.get_side());
//         });

//         auto zip = std::views::zip(moves, gains);
//         std::ranges::sort(zip, std::greater<>{}, [&](auto&& pair) {
//             return std::get<0>(pair) == best ? INT_MAX : std::get<1>(pair);
//         });

//         size_t length = 0;
//         bool pv_found = false;
//         size_t index = 0;
//         for (auto&& [move, gain] : zip) {

//             // if (depth == 1 && pv_found && !position.is_check() && position.get_material() + gain + 100 < alpha) {
//             //     break;  // pruning: skip moves that can't improve alpha
//             // }

//             if (!position.is_check() && pv_found && depth > 8 && index > 8 && gain <= 0) {
//                 break;
//             }

//             position.make_move(move);
//             result_t result;
//             if (pv_found) {
//                 // if (depth > 4 && index > 10) {
//                 //     result = -(*this)(-alpha - 1, -alpha, height + 1, depth - 1 - 2, pv_buffer);
//                 // } else {
//                     result = -(*this)(-alpha - 1, -alpha, height + 1, depth - 1, pv_buffer);
//                 // }
//                 if (result.score >= alpha) {
//                     result = -(*this)(-beta, -alpha, height + 1, depth - 1, pv_buffer);
//                 }
//             } else {
//                 result = -(*this)(-beta, -alpha, height + 1, depth - 1, pv_buffer);
//             }
//             position.undo_move(move);
//             history.put_all(move, height, position.get_side());

//             index++;

//             if (result.score >= beta) {
//                 transposition.put(position.hash(), move, beta, flag_t::LOWER, depth);
//                 history.put_good(move, height, position.get_side());
//                 pv.front() = move;
//                 std::ranges::copy(result.pv, pv.begin() + 1);
//                 return {beta, pv.first(result.pv.size() + 1)};
//             }

//             if (result.score > alpha) {
//                 alpha = result.score;
//                 best = move;
//                 pv_found = true;
//                 pv.front() = move;
//                 std::ranges::copy(result.pv, pv.begin() + 1);
//                 length = result.pv.size() + 1;
//             }
//         }

//         if (pv_found) {
//             transposition.put(position.hash(), best, alpha, flag_t::EXACT, depth);
//             history.put_good(best, height, position.get_side());
//         } else {
//             transposition.put(position.hash(), best, alpha, flag_t::UPPER, depth);
//         }

//         return {alpha, pv.first(length)};
//     }
// };

void demo_perft(position_t& position, int depth) {
    // position_t position{};
    perft_t perft{position};
    size_t total_nodes = 0;

    auto t0 = std::chrono::high_resolution_clock::now();
    for (int d = 1; d <= depth; ++d) {
        size_t nodes = perft(d);
        total_nodes += nodes;
        std::println("Depth {}: {} nodes", d, nodes);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = t1 - t0;

    std::println("Total nodes: {}, elapsed time: {:.2f} seconds, performance: {:.2f} Mnps", total_nodes, elapsed.count(), (total_nodes / elapsed.count()) / 1e6);
}

// void demo_search(position_t& position, int depth) {
//     searcher_t search{position};
//     std::array<move2_t, position_t::MAX_MOVES_PER_GAME> buffer;

//     auto t0 = std::chrono::high_resolution_clock::now();
//     for (int d = 1; d <= depth; ++d) {
//         auto [score, pv] = search(-30000, 30000, 0, d, buffer);
//         std::println("Depth {}/{}: {} nodes, score {}, pv {}", d, search.stats.max_height, search.stats.nodes, score, pv);
//         if (score < -29000 || score > 29000) {
//             break;
//         }
//     }
//     auto t1 = std::chrono::high_resolution_clock::now();
//     std::chrono::duration<double> elapsed = t1 - t0;

//     std::println("Total nodes: {}, elapsed time: {:.2f} seconds, performance: {:.2f} Mnps", 
//         search.stats.nodes, 
//         elapsed.count(), 
//         (search.stats.nodes / elapsed.count()) / 1e6);
// }

// void demo_search2(std::string_view fen, int depth) {
//     std::println("\ndepth: {}, fen: {}", depth, fen);

//     position_t position{fen};
//     searcher_t search{position};
//     std::array<move2_t, position_t::MAX_MOVES_PER_GAME> buffer;

//     auto t0 = std::chrono::high_resolution_clock::now();
//     for (int d = 1; d <= depth; ++d) {
//         auto [score, pv] = search(-30000, 30000, 0, d, buffer);
//         std::println("Depth {}/{}: {} nodes, score {}, pv {}", d, search.stats.max_height, search.stats.nodes, score, pv);
//         if (score < -29000 || score > 29000) {
//             break;
//         }
//     }
//     auto t1 = std::chrono::high_resolution_clock::now();
//     std::chrono::duration<double> elapsed = t1 - t0;

//     std::println("Total nodes: {}, elapsed time: {:.2f} seconds, performance: {:.2f} Mnps", 
//         search.stats.nodes, 
//         elapsed.count(), 
//         (search.stats.nodes / elapsed.count()) / 1e6);
// }

// struct foo_search {

//     transposition2_t transposition;
//     history2_t history;
//     evaluator2 evaluator;
//     std::array<move2_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;

//     void operator()(std::string_view fen, int depth) {
//         std::println("\ndepth: {}, fen: {}", depth, fen);

//         position_t position{fen};
//         searcher_t search{position, transposition, history, evaluator};

//         auto t0 = std::chrono::high_resolution_clock::now();
//         for (int d = 1; d <= depth; ++d) {
//             auto [score, pv] = search(-30000, 30000, 0, d, pv_buffer);
//             std::println("Depth {}/{}: {} nodes, score {}, pv {}", d, search.stats.max_height, search.stats.nodes, score, pv);
//             if (score < -29000 || score > 29000) {
//                 break;
//             }
//         }
//         auto t1 = std::chrono::high_resolution_clock::now();
//         std::chrono::duration<double> elapsed = t1 - t0;

//         std::println("Total nodes: {}, elapsed time: {:.2f} seconds, performance: {:.2f} Mnps", 
//             search.stats.nodes, 
//             elapsed.count(), 
//             (search.stats.nodes / elapsed.count()) / 1e6);

//         transposition.clear();
//         history.clear();
//     }

// };

void demo() {
    // position_t fen_position{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"sv};//"4k3/8/3PK3/8/8/8/8/8 b - - 0 128"sv};//"8/4B1k1/8/4N3/5K2/8/8/8 w - - 0 260"sv};

    // std::println("Side to move: {}", fen_position.get_side() == WHITE ? "white" : "black");
    // // std::println("Half move: {}", fen_position.get_half_move());
    // // std::println("Full move: {}", fen_position.get_full_move());
    // std::println("Material: {}", fen_position.get_material());
    // std::println("Hash: {}", fen_position.hash());
    // std::println("check: {}", fen_position.is_check());
    // std::println("50-move rule: {}", fen_position.is_50_moves_rule());
    // std::println("3-fold repetition: {}", fen_position.is_3_fold_repetition());

    // demo_perft(fen_position, 7);
    // demo_search(fen_position, 10);

    foo_search demo_search2;

    demo_search2("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"sv, 10);
    demo_search2("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"sv, 10);
    demo_search2("4k3/8/3PK3/8/8/8/8/8 b - -"sv, 18);
    demo_search2("8/4B1k1/8/4N3/5K2/8/8/8 w - -"sv, 13);
    demo_search2("1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - -"sv, 5);
    demo_search2("rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq -"sv, 9);
    demo_search2("r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w - -"sv, 8);
    demo_search2("1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w - -"sv, 8);
    demo_search2("4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w - -"sv, 9);
    demo_search2("3rr1k1/pp3pp1/1qn2np1/8/3p4/PP1R1P2/2P1NQPP/R1B3K1 b - -"sv, 8);
    demo_search2("r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1 b - -"sv, 2);
    demo_search2("r2q1rk1/4bppp/p2p4/2pP4/3pP3/3Q4/PP1B1PPP/R3R1K1 w - -"sv, 7);
    demo_search2("2r3k1/1p2q1pp/2b1pr2/p1pp4/6Q1/1P1PP1R1/P1PN2PP/5RK1 w - -"sv, 4);
    demo_search2("r1bqkb1r/4npp1/p1p4p/1p1pP1B1/8/1B6/PPPN1PPP/R2Q1RK1 w kq -"sv, 6);
    demo_search2("r2q1rk1/1ppnbppp/p2p1nb1/3Pp3/2P1P1P1/2N2N1P/PPB1QP2/R1B2RK1 b - -"sv, 6);
    demo_search2("r1bq1rk1/pp2ppbp/2np2p1/2n5/P3PP2/N1P2N2/1PB3PP/R1B1QRK1 b - -"sv, 7);
    demo_search2("3rr3/2pq2pk/p2p1pnp/8/2QBPP2/1P6/P5PP/4RRK1 b - -"sv, 6);
    demo_search2("3rn2k/ppb2rpp/2ppqp2/5N2/2P1P3/1P5Q/PB3PPP/3RR1K1 w - -"sv, 6);
    demo_search2("2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b - -"sv, 8);
    demo_search2("r2qnrnk/p2b2b1/1p1p2pp/2pPpp2/1PP1P3/PRNBB3/3QNPPP/5RK1 w - -"sv, 6);
    // demo_search2("6rr/1b2qk2/p3pn1p/3pQ1p1/3BP2P/5P1B/1PP5/2KR2R1 w - - 6 58"sv, 9);


    // size_t counter = 0;
    // perft_t perft{fen_position};
    // auto t0 = std::chrono::high_resolution_clock::now();
    // for (int depth = 0; depth < 8; ++depth) {
    //     size_t nodes = perft(depth);
    //     counter += nodes;
    //     std::println("Depth {}: {} moves", depth, nodes);
    // }
    // auto t1 = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double> elapsed = t1 - t0;
    // std::println("Total nodes: {}, elapsed time: {:.2f} seconds, performance: {:.2f} Mnps", counter, elapsed.count(), (counter / elapsed.count()) / 1e6);

    // position_t uci_position{"d2d4 g8f6 c2c4 e7e6 g1f3 b7b6 g2g3 c8a6 b2b3 f8b4 c1d2 b4e7 f1g2 c7c6 d2c3 d7d5 f3e5 f6d7 e5d7 b8d7 e1g1"sv};
    // int score = searcher_t{uci_position}(-10000, +10000, depth);
    // std::println("Best score at depth {}: {}", depth, score);

    static_assert(sizeof(position_t) == 168);
    static_assert(sizeof(position_t::state_t) == 40);
    static_assert(sizeof(perft_t) == 8);
    static_assert(sizeof(move2_t) == 3);
}

void uci_demo() {
    try {
    uci_interface uci;
    std::stringstream ss;
    ss << "uci\n";
    ss << "setoption name Hash value 16\n";
    ss << "isready\n";
    // ss << "position fen 2rq1rk1/1b2bp2/p3pn1p/np1p2p1/3P4/PPNQB1P1/3NPPBP/R1R3K1 b - - 9 20\n";
    ss << "position startpos moves e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 a7a6 c1e3 e7e6 f2f3 b7b5 g2g4 h7h6 d1d2 b8d7 e1c1 c8b7 a2a3\n";
    ss << "go wtime 300000 btime 300000 winc 1000 binc 1000 movestogo 40\n";
    // ss << "position startpos moves e2e4 e7e5 g1f3 b8c6 f1c4 f8c5 e1g1 g8f6 d2d3 e8g8 c2c3 d7d6 h2h3 h7h6 f1e1 a7a6 b1d2 c6a5 b2b4 a5c4 b4c5 c4a5 a2a4 f8e8 c5d6 c7d6 c3c4 f6d7 d2f1 d7c5 c1e3 a5c6 f1g3 d8a5 f3h4 c6b4 h4f5 b4d3 d1g4 g7g6 e1d1 a5d8 e3h6 b7b6 a1a3 d3f4 h6f4 e5f4 g4f4 c8f5 e4f5 g6g5 f4g4 f7f6 h3h4 a8a7 f2f4 a7h7 f4g5 f6g5 h4g5 h7g7 g5g6 g7d7 a3f3 e8e5 f5f6 c5e6 g3f5 d8f6 f5h6 g8g7 f3f6 g7f6 h6f7 e5f5 g6g7 e6g7 f7h6 d6d5 h6f5 g7f5 d1f1 d7f7 g4f5 f6e7 f5f7 e7d6 f7d5 d6c7\n";
    // ss << "go wtime 335600 btime 276834 winc 1000 binc 1000 movestogo 35\n";
    // ss << "position startpos moves e2e4 e7e5 g1f3 b8c6 d2d4 e5d4 f3d4 g8f6 b1c3 f8b4 d4c6 b7c6 f1d3 e8g8 e1g1 f8e8 c1g5 a8b8 f2f4 b4e7 e4e5 f6d5 d1h5 g7g6 h5h6 d5c3 b2c3 e7g5 f4g5 e8e5 f1f7 g8f7 h6h7 f7e6 h7g6 e6d5 g6f7 d5d6 h2h4 c6c5 f7f4 d8h8 g5g6 d6c6 a1f1 a7a6 f4g3 e5e7 g3g5 h8g7 h4h5 e7e8 h5h6 g7c3 g6g7 c3d4 g1h2 d7d6 d3g6 e8g8 g6f7 c8d7 f7g8 b8g8 g5f4 g8g7 f4d4 c5d4 h6g7 d7e6 a2a3 c6d7 f1f6 e6d5 f6f5 d5e6 f5f6 e6d5 f6f5 d5e6\n";
    // ss << "go wtime 11597800 btime 117601 winc 0 binc 0 movestogo 1\n";
    // ss << "stop\n";
    // ss << "quit\n";
    uci.run(ss);
    // uci.run(std::cin);
    } catch (const std::exception& e) {
        std::println("error: {}", e.what());
    }
}

int main() {
    uci_demo();
}
