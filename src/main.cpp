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
    mutable transposition_t transposition{99527};//15'485'863};
    mutable history_t history;

    struct result_t {
        std::int32_t score;
        move_t move;
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
    std::int32_t search(node& position, std::int32_t alpha, std::int32_t beta, int height) const noexcept {
        if (stop_token.stop_requested())
            return {};

        if (!position.checkers<Perspective>().empty()) {
            return search<Perspective>(position, alpha, beta, height, 1).score;
        }

        if (height > max_height)
            max_height = height;

        ++nodes;

        int standing_pat = evaluator.evaluate<Perspective>(position) * (76898 + position.material<Perspective>()) / 74411;
        // int standing_pat = (2 * position.material<Perspective>() + evaluator.evaluate<Perspective>(position)) / 2;// & ~15;
        // int standing_pat = position.material<Perspective>() + evaluator.evaluate<Perspective>(position) / 4;
        // int standing_pat = (position.material<Perspective>() + evaluator.evaluate<Perspective>(position) / 2) / 2;
        if (standing_pat >= beta)
            return beta;
        if (alpha < standing_pat)
            alpha = standing_pat;

        std::array<move_t, 256> buffer;
        auto moves = position.generate<Perspective, node::captures>(buffer);

        std::ranges::sort(moves, std::greater{}, [&](const move_t& move) {
            return evaluator.evaluate(position, move);
        });

        for (const auto& move : moves) {
            node successor{position};
            successor.execute<Perspective>(move);
            const int score = -search<~Perspective>(successor, -beta, -alpha, height + 1);

            if (stop_token.stop_requested())
                return {};

            if (score >= beta)
                return beta;
            if (score > alpha)
                alpha = score;
        }

        return alpha;
    }

    template <side_e Perspective>
    result_t search(/*const*/ node& position, std::int32_t alpha, std::int32_t beta, int height, std::int32_t depth) const noexcept {
        if (stop_token.stop_requested())
            return {};

        // repition detection
        for (auto* node = position.parent(); node != nullptr; node = node->parent())
            if (node->hash<Perspective>() == position.hash<Perspective>())
                return {0, {}};

        if (depth == 0)
            return result_t{search<Perspective>(position, alpha, beta, height), {}};

        if (height > max_height)
            max_height = height;

        ++nodes;

        const bool check = !position.checkers<Perspective>().empty();

        std::array<move_t, 256> buffer;
        auto moves = position.generate<Perspective, node::all>(buffer);

        if (moves.empty())
            return check ? result_t{MIN + height, {}} : result_t{0, {}};

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
            if (entry->depth >= depth)
            {
                switch (entry->flag)
                {
                case flag_t::EXACT:
                    return {entry->score, entry->move};
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
                if (alpha >= beta)
                    return {beta, entry->move};
            }
            best = entry->move;
        }

        // if (!check) {
        //     int score = position.material<Perspective>() - 120 * depth;
        //     if (score > beta)
        //     return {beta, best};
        // }

        const std::uint_fast8_t reduction = 2 + depth / 3;
        const bool skip = entry && entry->depth > depth - reduction && entry->score < beta && entry->flag == flag_t::UPPER;
        if (depth > reduction  && position.moved() != nullptr  && !check && !skip && try_null<Perspective>(position)) {
            auto [e, m] = position.execute(bitboard{}, nullptr);
            int score = -search<~Perspective>(position, -beta, -beta + 1, height + 1, depth - reduction).score;
            position.execute(e, m);
            if (score >= beta) {
                score = search<Perspective>(position, beta - 1, beta, height + 1, depth - 1).score;
                if (score >= beta)
                    return { beta, {} };
            }
        }

        // std::array<move_t, 256> buffer;
        // auto moves = position.generate<Perspective, node::all>(buffer);

        // if (moves.empty())
        //     return check ? result_t{MIN, {}} : result_t{0, {}};

        // if (best == move_t{} && depth > 2 && moves.size() > 1)
        //     best = search<Perspective>(position, alpha, beta, depth - 2).move;

        std::ranges::sort(moves, std::greater{}, [&](const move_t& move) -> std::int64_t {
            return move == best ? INT64_MAX : (int64_t{evaluator.evaluate(position, move)} << 32) + history.get<Perspective>(move);
        });

        bool pv = false;
        int index = 0;
        for (const auto& move : moves) {
            node successor{position};
            successor.execute<Perspective>(move);
            int score;
            if (!pv || check)
                score = -search<~Perspective>(successor, -beta, -alpha, height + 1, depth - 1).score;
            else {
                bool reduction = !check && depth >= 3 && index >= moves.size() / 4;
                score = -search<~Perspective>(successor, -alpha-1, -alpha, height + 1, depth - 1 - reduction * 2).score;
                if (score > alpha)// && score < beta)
                    score = -search<~Perspective>(successor, -beta, -alpha, height + 1, depth - 1).score;
            }

            if (stop_token.stop_requested())
                return {};

            if (score >= beta){
                transposition.put<Perspective>(position, move, beta, flag_t::LOWER, depth);
                history.put_good<Perspective>(move, depth);
                return result_t{beta, move};
            }
            if (score > alpha) {
                alpha = score;
                best = move;
                pv = true;
                if (height == 0)
                    // std::println("info depth {} seldepth {} score cp {} nodes {} pv {}", depth, max_height, score, nodes, best);
                    std::println("info depth {} seldepth {} score cp {} pv {}", depth, max_height, score, best);
            }
            ++index;
        }

        if (pv) {
            transposition.put<Perspective>(position, best, alpha, flag_t::EXACT, depth);
            history.put_good<Perspective>(best, depth);
        }
        else
            transposition.put<Perspective>(position, best, alpha, flag_t::UPPER, depth);

        return {alpha, best};
    }

    template <side_e Perspective>
    void search(node& position, std::int32_t depth) const noexcept {
        using as_floating_point = std::chrono::duration<double, std::ratio<1>>;
        auto time0 = std::chrono::high_resolution_clock::now();
        for (std::int32_t iteration = 1; iteration <= depth; ++iteration) {
            auto [score, move] = search<Perspective>(position, MIN, MAX, 0, iteration);
            if (stop_token.stop_requested())
                break;
            best = move;
            // std::println("{} {} {}", iteration, score, move);
            auto time1 = std::chrono::high_resolution_clock::now();
            auto time = duration_cast<as_floating_point>(time1 - time0).count();
            std::println("info depth {} seldepth {} multipv 1 score cp {} nodes {} nps {} hashfull {} tbhits {} time {} pv {}", iteration, max_height, score, nodes, size_t(nodes / time), transposition.full(), 0, size_t(time * 1000), best);
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
        std::array<char, 4024> buffer;
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
        root = node{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"sv, side};
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

    void position(std::string_view args) {
        static const std::regex re("position startpos moves (.*)");
        if (std::cmatch m; std::regex_match(&*args.begin(), &*args.end(), m, re)) {
            // std::println("xxxxx positioning matched");
            root = node{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"sv, side};
            if (m[1].matched) {
                for (auto x : std::views::split(m[1].str(), ' ')) {
                    std::string_view v {x};
                    // std::println("xxxxx executing '{}'", v);
                    (side == WHITE) ? root.execute<WHITE>(v) : root.execute<BLACK>(v);
                    side = ~side;
                }
            }
        }
    }

    void go(std::string_view args) {
        // static const std::regex re("go wtime (\\d*) btime (\\d*) winc (\\d*) binc (\\d*) (movestogo (\\d*))?");
        static const std::regex re("go wtime (\\d*) btime (\\d*) winc (\\d*) binc (\\d*).*");
        if (std::cmatch m; std::regex_match(&*args.begin(), &*args.end(), m, re)) {
            auto timeleft = std::stoll(m[side == WHITE ? 1 : 2].str());
            // auto movestogo = std::stoi(m[5].str());
            auto timetogo = timeleft * 4 / 100;
            searcher.clear();
            search = std::jthread{[&] {
                constexpr auto depth = 100;
                (side == WHITE) ? searcher.search<WHITE>(root, depth) : searcher.search<BLACK>(root, depth);
                std::println("bestmove {}", searcher.best);
                std::fflush(stdout);
            }};
            searcher.stop_token = search.get_stop_token();
            timer = std::jthread{[this, timetogo] {
                std::this_thread::sleep_for(timetogo * 1ms);
                if (!search.get_stop_token().stop_requested())
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
    node root{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", side};
};

int main() {
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
    // ss << "position startpos moves e2e4 e7e5 g1f3 b8c6 f1c4 f8c5 e1g1 g8f6 d2d3 e8g8 c2c3 d7d6 h2h3 h7h6 f1e1 a7a6 b1d2 c6a5 b2b4 a5c4 b4c5 c4d2 d1d2 f8e8 c1a3 b7b5 c5b6 c7c5 d3d4 d8b6 d4c5 d6c5 d2e3 c8b7 a3c5 b6c6 f3d2 f6h5 a1b1 h5f4 d2c4\n";
    // ss << "go wtime 49382 btime 48840 winc 1000 binc 1000 movestogo 20\n";
    // // ss << "stop\n";
    // // ss << "quit\n";
    // uci.run(ss);
    uci.run(std::cin);
}

/*

Result:
-----------------------------------------------------------------------------
  #  name             games    wins   draws  losses   score    elo    +    -
  1. Spike 1.4           12       8       3       1     9.5    159  156  132
  2. chess               12       5       6       1     8.0     82  133  124
  3. Hermann 2.8         12       6       2       4     7.0     48  142  135
  4. AnMon 5.75          12       4       4       4     6.0     14  132  132
  5. SOS 5 for Arena     12       4       4       4     6.0     -4  134  135
  6. Dragon 4.6          12       3       1       8     3.5   -117  137  157
  7. Nejmet_3.07         12       1       2       9     2.0   -181  139  172

Cross table:
-----------------------------------------------------------------------------
  #  name                score   games         1         2         3         4         5         6         7
  1. Spike 1.4             9.5      12         x       1.0       2.0       1.5       1.0       2.0       2.0
  2. chess                 8.0      12       1.0         x       0.5       1.5       1.5       2.0       1.5
  3. Hermann 2.8           7.0      12       0.0       1.5         x       0.5       1.0       2.0       2.0
  4. AnMon 5.75            6.0      12       0.5       0.5       1.5         x       0.5       1.0       2.0
  5. SOS 5 for Arena       6.0      12       1.0       0.5       1.0       1.5         x       1.5       0.5
  6. Dragon 4.6            3.5      12       0.0       0.0       0.0       1.0       0.5         x       2.0
  7. Nejmet_3.07           2.0      12       0.0       0.5       0.0       0.0       1.5       0.0         x

Tech:
-----------------------------------------------------------------------------

Tech (average nodes, depths, time/m per move, others per game), counted for computing moves only, ignored moves with zero nodes:
  #  name               nodes/m         NPS  depth/m   time/m    moves     time
  1. Spike 1.4           22248K     2089898     20.1     10.6     52.2    555.3
  2. chess                1642K      186164     10.1      8.8     51.9    457.8
  3. Hermann 2.8         14568K     1552030     14.4      9.4     69.8    655.5
  4. AnMon 5.75       -6492710838775K-690295355092575     14.3      9.4     63.0    592.6
  5. SOS 5 for Arena     19379K     2148747     18.2      9.0     75.7    682.4
  6. Dragon 4.6          24660K     2618404     12.3      9.4     65.4    616.1
  7. Nejmet_3.07         14226K     1489213     11.8      9.6     60.6    578.7
     all ---          -910782177699K-98840581839556     14.6      9.4     62.7    591.2

*/