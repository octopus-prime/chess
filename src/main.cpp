#include "square.hpp"
#include "piece.hpp"
#include "bitboard.hpp"
#include "move.hpp"
#include "node.hpp"
#include "evaluator.hpp"
#include "transposition.hpp"
#include "history.hpp"

#include <functional>
#include <iostream>
#include <regex>
#include <map>
#include <print>
#include <sstream>

#include <chrono>


class searcher {
    evaluator evaluator;
    mutable transposition_t transposition{1'000'037};
    mutable history_t history;

    struct result_t {
        std::int32_t score;
        std::span<move_t> pv;

        result_t operator-() const noexcept {
            return {-score, pv};
        }
    };

    constexpr static std::int16_t MIN = -32000;
    constexpr static std::int16_t MAX = +32000;

public:
    mutable std::size_t nodes = 0;
    mutable move_t best;
    mutable std::stop_token stop_token;
    mutable std::size_t max_height = 0;

    void clear() {
        transposition.clear();
        history.clear();
        nodes = 0;
        max_height = 0;
        best = {};
    }

    template <side_e side>
    inline bool
    try_null(const node& node) const noexcept {
        return node.occupied<side>().size() > 3
            && (node.knight<side>() | node.bishop_queen<side>() | node.rook_queen<side>());
    //		&& !(node.attack<other_tag>() & detail::attacker<king_tag, color_tag>::attack(node.occupy<king_tag, color_tag>()));
    }

    template <side_e Perspective>
    result_t search(node& position, std::int32_t alpha, std::int32_t beta, int height, std::span<move_t, 1024> pv) const noexcept {
        if (stop_token.stop_requested())
            return {};

        // if (!position.checkers<Perspective>().empty()) {
        //     return search<Perspective>(position, alpha, beta, height, 1, pv);
        // }

        if (height > max_height)
            max_height = height;

        ++nodes;

        // bool small = std::abs(position.material<Perspective>()) > 200;
        // int standing_pat = small
        // ? evaluator.evaluate2<Perspective>(position) * (76898 + position.material<Perspective>() * 2) / 74411
        // : evaluator.evaluate<Perspective>(position) * (76898 + position.material<Perspective>() * 2) / 74411;

        int standing_pat = evaluator.evaluate<Perspective>(position) * (76898 + position.material<Perspective>() * 2) / 74411;

        // int standing_pat = (position.material<Perspective>() + evaluator.evaluate<Perspective>(position)) / 2;// & ~15;
        // int standing_pat = position.material<Perspective>() + evaluator.evaluate<Perspective>(position) / 4;
        // int standing_pat = (position.material<Perspective>() + evaluator.evaluate<Perspective>(position) / 2) / 2;
        if (standing_pat >= beta)
            return {beta, {}};
        if (alpha < standing_pat)
            alpha = standing_pat;

        // constexpr int futility_margin = 900;
        // if (standing_pat + futility_margin < alpha)
        //     return {standing_pat, {}};

        std::array<move_t, 256> buffer;
        auto moves = position.generate<Perspective, node::captures>(buffer);

        std::ranges::sort(moves, std::greater{}, [&](const move_t& move) {
            return evaluator.evaluate(position, move);
        });

        move_t sub_pv[1024];
        size_t length = 0;

        bool pv_found = false;
        for (const auto& move : moves) {
            node successor{position};
            successor.execute<Perspective>(move);
            result_t result;

            if (stop_token.stop_requested())
                return {};
            
            if (pv_found) {
                result = -search<~Perspective>(successor, -alpha - 1, -alpha, height + 1, sub_pv);
                if (result.score > alpha && result.score < beta)
                    result = -search<~Perspective>(successor, -beta, -alpha, height + 1, sub_pv);
            } else {
                result = -search<~Perspective>(successor, -beta, -alpha, height + 1, sub_pv);
            }
            // result = -search<~Perspective>(successor, -beta, -alpha, height + 1, sub_pv);

            if (stop_token.stop_requested())
                return {};

            if (result.score >= beta) {
                pv.front() = move;
                std::ranges::copy(result.pv, pv.begin() + 1);
                return {beta, pv.first(result.pv.size() + 1)};
            }
            if (result.score > alpha) {
                alpha = result.score;
                pv_found = true;
                pv.front() = move;
                length = result.pv.size() + 1;
                std::ranges::copy(result.pv, pv.begin() + 1);
            }
        }

        return {alpha, pv.first(length)};
    }

    template <side_e Perspective>
    result_t search(/*const*/ node& position, std::int32_t alpha, std::int32_t beta, int height, std::int32_t depth, std::span<move_t, 1024> pv) const noexcept {
        if (stop_token.stop_requested())
            return {};

        // 50 moves rule
        if (position.get_rule50() >= 100)
            return {0, {}};

        // repition detection
        for (auto* node = position.parent(); node != nullptr; node = node->parent())
            if (node->hash<Perspective>() == position.hash<Perspective>()) {
                // std::println("repition detected {}", *position.moved());
                return {0, {}};
            }

        const bool check = !position.checkers<Perspective>().empty();

        if (depth == 0 && check)
            ++depth;

        if (depth == 0)
            return search<Perspective>(position, alpha, beta, height, pv);

        if (height > max_height)
            max_height = height;

        ++nodes;

        // const bool check = !position.checkers<Perspective>().empty();

        std::array<move_t, 256> buffer;
        auto moves = position.generate<Perspective, node::all>(buffer);

        if (moves.empty())
            return {check ? MIN + height : 0, {}};

        // depth += check;

        // // repition detection
        // for (auto* node = position.parent(); node != nullptr; node = node->parent())
        //     if (node->hash<Perspective>() == position.hash<Perspective>())
        //         return {0, {}};

        // if (depth == 0)
        //     return result_t{search<Perspective>(position, alpha, beta, height), {}};

        move_t best;
        const entry_t* const entry = transposition.get<Perspective>(position);
        if (entry)
        {
            best = entry->move;
            if (entry->depth >= depth)
            {
                switch (entry->flag)
                {
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

        // if (!check) {
        //     int score = position.material<Perspective>() - 120 * depth;
        //     if (score > beta)
        //     return {beta, best};
        // }

        move_t sub_pv[1024];
        size_t length = 0;

        // const std::uint_fast8_t reduction = 2 + depth / 3;
        // const bool skip = entry && entry->depth > depth - reduction && entry->score < beta && entry->flag == flag_t::UPPER;
        // if (height > 1 && depth > reduction  && position.moved() != nullptr  && !check && !skip && try_null<Perspective>(position)) {
        //     auto [e, m] = position.execute(bitboard{}, nullptr);
        //     int score = -search<~Perspective>(position, -beta, -beta + 1, height + 1, depth - reduction, sub_pv).score;
        //     position.execute(e, m);
        //     if (score >= beta) {
        //         score = search<Perspective>(position, beta - 1, beta, height + 1, depth - 1, sub_pv).score;
        //         if (score >= beta)
        //             return { beta, {} };
        //     }
        // }

        // std::array<move_t, 256> buffer;
        // auto moves = position.generate<Perspective, node::all>(buffer);

        // if (moves.empty())
        //     return check ? result_t{MIN, {}} : result_t{0, {}};

        // if (best == move_t{} && depth > 2 && moves.size() > 1)
        //     best = search<Perspective>(position, alpha, beta, depth - 2).move;

        std::ranges::sort(moves, std::greater{}, [&](const move_t& move) -> std::int64_t {
            return move == best ? INT64_MAX : (int64_t{evaluator.evaluate(position, move)} << 32) + history.get<Perspective>(move);
            // return move == best ? INT64_MAX : int64_t{evaluator.evaluate(position, move)} + history.get<Perspective>(move);
        });

        bool pv_found = false;
        int index = 0;
        for (const auto& move : moves) {
            if (height == 1 && depth > 6) {
                std::println("info currmove {} currmovenumber {}", move, index + 1);
                std::fflush(stdout);
            }
            node successor{position};
            successor.execute<Perspective>(move);
            result_t result;

            if (stop_token.stop_requested())
                return {};

            if (!pv_found || check)
                result = -search<~Perspective>(successor, -beta, -alpha, height + 1, depth - 1, sub_pv);
            else {
//                bool reduction = !check && depth >= 3 && index >= moves.size() / 4;
                // int max_reduction = depth / 3;
                int reduction = index * depth / (3 * moves.size());
                result = -search<~Perspective>(successor, -alpha-1, -alpha, height + 1, depth - 1 - reduction, sub_pv);
                if (result.score > alpha && result.score < beta)
                    result = -search<~Perspective>(successor, -beta, -alpha, height + 1, depth - 1, sub_pv);
                    // result = -search<~Perspective>(successor, -beta, -result.score, height + 1, depth - 1, sub_pv);
            }

            if (stop_token.stop_requested())
                return {};

            if (result.score >= beta) {
                transposition.put<Perspective>(position, move, beta, flag_t::LOWER, depth);
                history.put_good<Perspective>(move, depth);
                pv.front() = move;
                std::ranges::copy(result.pv, pv.begin() + 1);
                return {beta, pv.first(result.pv.size() + 1)};
            }
            if (result.score > alpha) {
                alpha = result.score;
                best = move;
                pv_found = true;
                pv.front() = move;
                length = result.pv.size() + 1;
                std::ranges::copy(result.pv, pv.begin() + 1);
            }
            ++index;
        }

        if (pv_found) {
            transposition.put<Perspective>(position, best, alpha, flag_t::EXACT, depth);
            history.put_good<Perspective>(best, depth);
        }
        else
            transposition.put<Perspective>(position, best, alpha, flag_t::UPPER, depth);

        return {alpha, pv.first(length)};
    }

    template <side_e Perspective>
    void search(node& position, std::int32_t depth) const noexcept {
        using as_floating_point = std::chrono::duration<double, std::ratio<1>>;
        move_t pv[1024];
        auto time0 = std::chrono::high_resolution_clock::now();
        int32_t score = search<Perspective>(position, MIN, MAX, 1, pv).score;
        for (std::int32_t iteration = 1; iteration <= depth; ++iteration) {
            // std::println("info depth {}", iteration);
            result_t result;
            // int32_t score;
            // move_t move;
            // auto [score, move] = search<Perspective>(position, MIN, MAX, 0, iteration);
            result = search<Perspective>(position, score - 100, score + 100, 1, iteration, pv);
            if (stop_token.stop_requested())
                break;
            if (result.score <= score - 100) {
                // auto time1 = std::chrono::high_resolution_clock::now();
                // auto time = duration_cast<as_floating_point>(time1 - time0).count();
                // std::println("info depth {} seldepth {} score cp {} nodes {} nps {} hashfull {} time {} pv{}", iteration, max_height, result.score, nodes, size_t(nodes / time), transposition.full(), size_t(time * 1000), result.pv);
                // std::fflush(stdout);
                result = search<Perspective>(position, MIN, result.score, 1, iteration, pv);
            } else if (result.score >= score + 100) {
                // auto time1 = std::chrono::high_resolution_clock::now();
                // auto time = duration_cast<as_floating_point>(time1 - time0).count();
                // std::println("info depth {} seldepth {} score cp {} nodes {} nps {} hashfull {} time {} pv{}", iteration, max_height, result.score, nodes, size_t(nodes / time), transposition.full(), size_t(time * 1000), result.pv);
                // std::fflush(stdout);
                result = search<Perspective>(position, result.score, MAX, 1, iteration, pv);
            }
            if (stop_token.stop_requested())
                break;
            best = result.pv.front();
            score = result.score;
            // std::println("{} {} {}", iteration, score, move);
            auto time1 = std::chrono::high_resolution_clock::now();
            auto time = duration_cast<as_floating_point>(time1 - time0).count();
            std::println("info depth {} seldepth {} score cp {} nodes {} nps {} hashfull {} time {} pv{}", iteration, max_height, result.score, nodes, size_t(nodes / time), transposition.full(), size_t(time * 1000), result.pv);
            // std::flush(std::cout);
            std::fflush(stdout);

            // if (score >= MAX - 100 || score <= MIN + 100)
            //     break;
        }
    }
};

struct uci_interface {

    uci_interface()
    : options {
        {"Hash", "1"}
    }, 
    commands {
        {"ucinewgame"sv, std::bind(&uci_interface::ucinewgame, this, std::placeholders::_1)},
        {"uci"sv, std::bind(&uci_interface::uci, this, std::placeholders::_1)},
        {"setoption"sv, std::bind(&uci_interface::setoption, this, std::placeholders::_1)},
        {"isready"sv, std::bind(&uci_interface::isready, this, std::placeholders::_1)},
        {"position"sv, std::bind(&uci_interface::position, this, std::placeholders::_1)},
        {"go"sv, std::bind(&uci_interface::go, this, std::placeholders::_1)},
        {"stop"sv, std::bind(&uci_interface::stop, this, std::placeholders::_1)},
        {"quit"sv, std::bind(&uci_interface::quit, this, std::placeholders::_1)},
    } {}

    void run(std::istream& stream) {
        std::array<char, 4096> buffer;
        for(;;) {
            stream.getline(buffer.data(), buffer.size());
            const std::string_view line{buffer.data()};
            const auto predicate = [line](const auto& command) { return line.starts_with(command.first); };
            if (const auto& command = std::ranges::find_if(commands, predicate); command != std::end(commands))
                command->second(line);
        }
    }

private:
    void uci(std::string_view) {
        std::println("id name chess");
        std::println("id author me");
        std::println("option name Hash type spin default 1 min 1 max 16");
        std::println("uciok");
    }

    void setoption(std::string_view args) {
        static const std::regex re("setoption name (\\w+) value (\\w+)");
        if (std::cmatch m; std::regex_match(&*args.begin(), &*args.end(), m, re)) {
            options[m[1].str()] = m[2].str();
        }
    }

    void isready(std::string_view) {
        std::println("hash = {}", options["Hash"]);
        std::println("readyok");
    }

    void ucinewgame(std::string_view) {
        nodes[0] = node{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"sv, side};
        root = &nodes[0];
    }

    // void position(std::string_view args) {
    //     static const std::regex re("position fen (.*)");
    //     if (std::cmatch m; std::regex_match(&*args.begin(), &*args.end(), m, re)) {
    //         root = node{m[1].str(), side};
    //     }
    // }

    // void position(std::string_view args) {
    //     static const std::regex re("^position (startpos|fen )(.*)( moves( \\w)+)?$");
    //     if (std::cmatch m; std::regex_match(&*args.begin(), &*args.end(), m, re)) {
    //         auto s = m[2].str();
    //         if (s.empty())
    //             s = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    //         root = node{s, side};
    //         if (m[3].matched) {
    //             side_e side_ = side;
    //             for (auto x : std::views::split(m[3].str(), ' ')) {
    //                 std::string_view v {x};
    //                 std::println("executing '{}'", v);
    //                 (side_ == WHITE) ? root.execute<WHITE>(v) : root.execute<BLACK>(v);
    //                 side_ = ~side_;
    //             }
    //         }
    //     }
    // }

    // void position(std::string_view args) {
    //     static const std::regex re("position startpos moves (.*)");
    //     if (std::cmatch m; std::regex_match(&*args.begin(), &*args.end(), m, re)) {
    //         // std::println("xxxxx positioning matched");
    //         root = node{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"sv, side};
    //         if (m[1].matched) {
    //             for (auto x : std::views::split(m[1].str(), ' ')) {
    //                 std::string_view v {x};
    //                 // std::println("xxxxx executing '{}'", v);
    //                 (side == WHITE) ? root.execute<WHITE>(v) : root.execute<BLACK>(v);
    //                 side = ~side;
    //             }
    //         }
    //     }
    // }

    void position(std::string_view args) {
        static const std::regex re("position startpos( moves (.*))?");
        if (std::cmatch m; std::regex_match(&*args.begin(), &*args.end(), m, re)) {
            // std::println("xxxxx positioning matched");
            nodes[0] = node{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"sv, side};
            root = &nodes[0];
            if (m[2].matched) {
                for (auto [i, x] : std::views::enumerate(std::views::split(m[2].str(), ' '))) {
                    root = &nodes[i + 1];
                    *root = nodes[i];
                    std::string_view v {x};
                    // std::println("xxxxx executing '{}'", v);
                    (side == WHITE) ? root->execute<WHITE>(v) : root->execute<BLACK>(v);
                    side = ~side;
                }
            }
        }
    }

    void go(std::string_view args) {
        static const std::regex re("go wtime (\\d*) btime (\\d*) winc (\\d*) binc (\\d*)( movestogo (\\d*))?");
        if (std::cmatch m; std::regex_match(&*args.begin(), &*args.end(), m, re)) {
            auto timeleft = std::stoll(m[side == WHITE ? 1 : 2].str());
            auto timeinc = std::stoll(m[side == WHITE ? 3 : 4].str());
            long timetogo = timeinc;
            if (m[6].matched) {
                auto movestogo = std::stoll(m[6].str()) + 1;
                timetogo += timeleft / movestogo;
            } else
                timetogo += timeleft * 5 / 100;
            searcher.clear();
            search = std::jthread{[&] {
                constexpr auto depth = 100;
                (side == WHITE) ? searcher.search<WHITE>(*root, depth) : searcher.search<BLACK>(*root, depth);
                std::println("bestmove {}", searcher.best);
                std::fflush(stdout);
            }};
            searcher.stop_token = search.get_stop_token();
            timer = std::jthread{[this, timetogo] {
                std::this_thread::sleep_for(timetogo * 1ms);
                // if (!search.get_stop_token().stop_requested())
                    stop(""sv);
            }};
            if (timer.joinable())
                timer.join();
        }
    }

    void stop(std::string_view) {
        search.request_stop();
        if (search.joinable())
            search.join();
    }

    void quit(std::string_view args) {
        stop(args);
        std::exit(0);
    }

    std::map<std::string, std::string> options;
    std::map<std::string_view, std::function<void(std::string_view)>> commands;

    searcher searcher;
    std::jthread search;
    std::jthread timer;

    side_e side;
    // node root{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", side};

    std::vector<node> nodes{1024};
    node* root;
};

int main() {
    try {
    uci_interface uci;
    // std::stringstream ss;
    // ss << "uci\n";
    // ss << "setoption name Hash value 16\n";
    // ss << "isready\n";
    // // ss << "position fen 2rq1rk1/1b2bp2/p3pn1p/np1p2p1/3P4/PPNQB1P1/3NPPBP/R1R3K1 b - - 9 20\n";
    // // ss << "position startpos moves e2e4 e7e5\n";
    // // ss << "go wtime 300000 btime 300000 winc 1000 binc 1000 movestogo 40\n";
    // // ss << "position startpos moves e2e4 e7e5 g1f3 b8c6 f1c4 f8c5 e1g1 g8f6 d2d3 e8g8 c2c3 d7d6 h2h3 h7h6 f1e1 a7a6 b1d2 c6a5 b2b4 a5c4 b4c5 c4a5 a2a4 f8e8 c5d6 c7d6 c3c4 f6d7 d2f1 d7c5 c1e3 a5c6 f1g3 d8a5 f3h4 c6b4 h4f5 b4d3 d1g4 g7g6 e1d1 a5d8 e3h6 b7b6 a1a3 d3f4 h6f4 e5f4 g4f4 c8f5 e4f5 g6g5 f4g4 f7f6 h3h4 a8a7 f2f4 a7h7 f4g5 f6g5 h4g5 h7g7 g5g6 g7d7 a3f3 e8e5 f5f6 c5e6 g3f5 d8f6 f5h6 g8g7 f3f6 g7f6 h6f7 e5f5 g6g7 e6g7 f7h6 d6d5 h6f5 g7f5 d1f1 d7f7 g4f5 f6e7 f5f7 e7d6 f7d5 d6c7\n";
    // // ss << "go wtime 335600 btime 276834 winc 1000 binc 1000 movestogo 35\n";
    // ss << "position startpos moves e2e4 e7e5 g1f3 b8c6 d2d4 e5d4 f3d4 g8f6 b1c3 f8b4 d4c6 b7c6 f1d3 e8g8 e1g1 f8e8 c1g5 a8b8 f2f4 b4e7 e4e5 f6d5 d1h5 g7g6 h5h6 d5c3 b2c3 e7g5 f4g5 e8e5 f1f7 g8f7 h6h7 f7e6 h7g6 e6d5 g6f7 d5d6 h2h4 c6c5 f7f4 d8h8 g5g6 d6c6 a1f1 a7a6 f4g3 e5e7 g3g5 h8g7 h4h5 e7e8 h5h6 g7c3 g6g7 c3d4 g1h2 d7d6 d3g6 e8g8 g6f7 c8d7 f7g8 b8g8 g5f4 g8g7 f4d4 c5d4 h6g7 d7e6 a2a3 c6d7 f1f6 e6d5 f6f5 d5e6 f5f6 e6d5 f6f5 d5e6\n";
    // ss << "go wtime 11597800 btime 117601 winc 0 binc 0 movestogo 11\n";
    // // ss << "stop\n";
    // // ss << "quit\n";
    // uci.run(ss);
    uci.run(std::cin);
    } catch (const std::exception& e) {
        std::println("error: {}", e.what());
    }
}
