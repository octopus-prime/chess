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

// struct uci_interface {

//     uci_interface()
//     : options {
//         {"Hash", "1"}
//     }, 
//     commands {
//         {"uci"sv, std::bind(&uci_interface::uci, this, std::placeholders::_1)},
//         {"setoption"sv, std::bind(&uci_interface::setoption, this, std::placeholders::_1)},
//         {"isready"sv, std::bind(&uci_interface::isready, this, std::placeholders::_1)},
//         {"ucinewgame"sv, [](std::string_view) {}},
//         {"position"sv, [](std::string_view args) {}},
//         {"go"sv, [](std::string_view args) {}},
//         {"quit"sv, [](std::string_view) { std::exit(0); }},
//     } {}

//     void run(std::istream& stream) {
//         std::array<char, 1024> buffer;
//         for(;;) {
//             stream.getline(buffer.data(), buffer.size());
//             const std::string_view line{buffer.data()};
//             const auto predicate = [line](const auto& command) { return line.starts_with(command.first); };
//             if (const auto& command = std::ranges::find_if(commands, predicate); command != std::end(commands))
//                 command->second(line);
//         }
//     }

// private:
//     void uci(std::string_view) {
//         std::println("id name chess");
//         std::println("id author me");
//         std::println("option name Hash type spin default 1 min 1 max 16");
//         std::println("uciok");
//     }

//     void setoption(std::string_view args) {
//         static const std::regex re("setoption name (\\w+) value (\\w+)");
//         std::cmatch m;
//         if (std::regex_match(&*args.begin(), &*args.end(), m, re)) {
//             options[m[1].str()] = m[2].str();
//         }
//     }

//     void isready(std::string_view) {
//         std::println("hash = {}", options["Hash"]);
//         std::println("readyok");
//     }

//     void ucinewgame(std::string_view) {
//     }

//     void position(std::string_view args) {
//     }

//     std::map<std::string, std::string> options;
//     std::map<std::string_view, std::function<void(std::string_view)>> commands;
// };

// int main() {
//     uci_interface uci;
//     std::stringstream ss;
//     ss << "uci\n";
//     ss << "setoption name Hash value 16\n";
//     ss << "isready\n";
//     ss << "quit\n";
//     uci.run(ss);
//     // uci.run(std::cin);
// }


// class table {
//     struct entry {
//         uint64_t hash = -1;
//         size_t count = 0;
//         int depth = -1;
//     };

//     std::vector<entry> entries_;

// public:
//     // constexpr table() : entries_() { /*clear();*/ }
//     table(std::size_t size) : entries_(size) { /*clear();*/ }

//     // void clear() noexcept {
//     //     for (entry& e : entries_) {
//     //         e.hash = -1;
//     //         e.count = 0;
//     //         e.depth = 0;
//     //     }
//     // }

//     // constexpr bool empty() const noexcept {
//     //     return entries_.empty();
//     // }

//     double full() const noexcept {
//         return 100.0 * std::ranges::count_if(entries_, [](auto&& e) { return e.hash != -1; }) / entries_.size();
//     }

//     template <side_e side>
//     void put(const node &current, int depth, std::size_t count) noexcept {
//         auto hash = current.hash<side>();
//         entry& e = entries_[hash % entries_.size()];
//         if (e.depth <= depth && e.count < count)
//             e = {hash, count, depth};
//     }

//     template <side_e side>
//     std::optional<std::size_t> get(const node &current, int depth) const noexcept {
//         auto hash = current.hash<side>();
//         const entry& e = entries_[hash % entries_.size()];
//         return e.hash == hash && e.depth == depth ? std::make_optional(e.count) : std::nullopt;
//     }
// };

// table table_{15'485'863};

// template <side_e side, bool divide>
// std::size_t perft(const node &current, int depth) noexcept
// {
//     if (auto&& hit = table_.get<side>(current, depth); hit.has_value())
//         return hit.value();

//     std::size_t count = 0;
//     std::array<move_t, 256> buffer;
//     auto moves = current.generate<side, node::all>(buffer);
//     for (const auto &move : moves)
//     {
//         std::size_t count_ = 0;
//         if (depth <= 1)
//             count_ += 1;
//         else
//         {
//             node succ(current);
//             succ.execute<side>(move);
//             std::array<move_t, 256> buffer2;
//             count_ += depth == 2 ? succ.generate<~side, node::all>(buffer2).size() : perft<~side, false>(succ, depth - 1);
//         }
//         count += count_;
//         if constexpr (divide) {
//             std::println("{}{:16L}", move, count_);
//         }
//     }

//     table_.put<side>(current, depth, count);

//     return count;
// }

// int main() {
//     using as_floating_point = std::chrono::duration<double, std::ratio<1>>;

//     side_e side = WHITE;
//     // const node current {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"sv, side};
//     const node current {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"sv, side};
//     auto time0 = std::chrono::high_resolution_clock::now();
//     std::size_t counter_ = perft<WHITE, true>(current, 7);
//     auto time1 = std::chrono::high_resolution_clock::now();
//     auto time = duration_cast<as_floating_point>(time1 - time0).count();
//     std::println("{:7.3f} {:16L} {:16L}", time, counter_, size_t(counter_ / time));
//     std::println("table = {:7.3f}", table_.full());
// }

class searcher {
    evaluator evaluator;
    mutable transposition_t transposition{15'485'863};
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

    template <side_e side>
    inline bool
    try_null(const node& node) const noexcept {
        return node.occupied<side>().size() > 3
            && (node.knight<side>() | node.bishop_queen<side>() | node.rook_queen<side>());
    //		&& !(node.attack<other_tag>() & detail::attacker<king_tag, color_tag>::attack(node.occupy<king_tag, color_tag>()));
    }

    template <side_e Perspective>
    std::int32_t search(const node& position, std::int32_t alpha, std::int32_t beta) const noexcept {
        if (stop_token.stop_requested())
            return {};

        ++nodes;

        int standing_pat = evaluator.evaluate<Perspective>(position); // * (76898 + position.material<Perspective>()) / 74411;
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
            const int score = -search<~Perspective>(successor, -beta, -alpha);

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
    result_t search(/*const*/ node& position, std::int32_t alpha, std::int32_t beta, std::int32_t depth) const noexcept {
        if (stop_token.stop_requested())
            return {};

        ++nodes;

        const bool check = !position.checkers<Perspective>().empty();

        depth += check;

        if (depth == 0)
            return result_t{search<Perspective>(position, alpha, beta), {}};

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

        if (!check) {
            int score = position.material<Perspective>() - 120 * depth;
            if (score > beta)
            return {beta, best};
        }

        const std::uint_fast8_t reduction = 1 + depth / 3;
        const bool skip = entry && entry->depth > depth - reduction && entry->score < beta && entry->flag == flag_t::UPPER;
        if (depth > reduction  && position.moved() != nullptr  && !check && !skip && try_null<Perspective>(position)) {
            auto [e, m] = position.execute(bitboard{}, nullptr);
            int score = -search<~Perspective>(position, -beta, -beta + 1, depth - reduction).score;
            position.execute(e, m);
            if (score >= beta) {
                score = search<Perspective>(position, beta - 1, beta, depth - 1).score;
                if (score >= beta)
                    return { beta, {} };
            }
        }

        std::array<move_t, 256> buffer;
        auto moves = position.generate<Perspective, node::all>(buffer);

        if (moves.empty())
            return check ? result_t{MIN, {}} : result_t{0, {}};

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
                score = -search<~Perspective>(successor, -beta, -alpha, depth - 1).score;
            else {
                bool reduction = !check && depth >= 3 && index >= moves.size() / 3;
                score = -search<~Perspective>(successor, -alpha-1, -alpha, depth - 1 - reduction * 2).score;
                if (score > alpha)// && score < beta)
                    score = -search<~Perspective>(successor, -beta, -alpha, depth - 1).score;
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
            auto [score, move] = search<Perspective>(position, MIN, MAX, iteration);
            if (stop_token.stop_requested())
                break;
            best = move;
            // std::println("{} {} {}", iteration, score, move);
            auto time1 = std::chrono::high_resolution_clock::now();
            auto time = duration_cast<as_floating_point>(time1 - time0).count();
            std::println("info depth {} seldepth {} multipv 1 score cp {} nodes {} nps {} hashfull {} tbhits {} time {} pv {}", iteration, 0, score, nodes, size_t(nodes / time), 0, 0, size_t(time * 1000), best);
        }
    }
};

// int main() {
//     using as_floating_point = std::chrono::duration<double, std::ratio<1>>;

//     constexpr auto depth = 12;
//     side_e side;
//     searcher searcher{};
//     // node current {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"sv, side};
//     // node current {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"sv, side};
//     // node current {"8/2Nb4/pp6/4rp1p/1Pp1pPkP/PpPpR3/1B1P2N1/1K6 w - -"sv, side};
//     // node current {"2r3k1/pppR1pp1/4p3/4P1P1/5P2/1P4K1/P1P5/8 w - -"sv, side};
//     // node current {"2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b - -"sv, side};
//     // node current {"1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - -"sv, side};
//     // node current {"3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w - -"sv, side};
//     // node current {"2q1rr1k/3bbnnp/p2p1pp1/2pPp3/PpP1P1P1/1P2BNNP/2BQ1PRK/7R b - -"sv, side};
//     // node current {"rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq -"sv, side};
//     node current {"r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w - -"sv, side};
//     auto time0 = std::chrono::high_resolution_clock::now();
//     // auto score = 
//     side == WHITE ? searcher.search<WHITE>(current, depth) : searcher.search<BLACK>(current, depth);
//     auto time1 = std::chrono::high_resolution_clock::now();
//     auto time = duration_cast<as_floating_point>(time1 - time0).count();
//     std::println("{:7.3f} {:16L} {:16L}", time, searcher.nodes, size_t(searcher.nodes / time));
//     // std::println("table = {:7.3f}", table_.full());
// }

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
        std::array<char, 1024> buffer;
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
    }

    void position(std::string_view args) {
        static const std::regex re("position fen (.*)");
        if (std::cmatch m; std::regex_match(&*args.begin(), &*args.end(), m, re)) {
            root = node{m[1].str(), side};
        }
    }

    void go(std::string_view args) {
        static const std::regex re("go wtime (\\d*) btime (\\d*) winc (\\d*) binc (\\d*) movestogo (\\d*)");
        if (std::cmatch m; std::regex_match(&*args.begin(), &*args.end(), m, re)) {
            search = std::jthread{[&] {
                constexpr auto depth = 10;
                (side == WHITE) ? searcher.search<WHITE>(root, depth) : searcher.search<BLACK>(root, depth);
                std::println("bestmove {}", searcher.best);
            }};
            searcher.stop_token = search.get_stop_token();
        }
    }

    void stop(std::string_view) {
        // search.request_stop();
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

    side_e side;
    node root{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", side};
};

int main() {
    uci_interface uci;
    std::stringstream ss;
    ss << "uci\n";
    ss << "setoption name Hash value 16\n";
    ss << "isready\n";
    ss << "position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 2\n";
    ss << "go wtime 300000 btime 300000 winc 1000 binc 1000 movestogo 40\n";
    ss << "stop\n";
    ss << "quit\n";
    uci.run(ss);
    // uci.run(std::cin);
}
