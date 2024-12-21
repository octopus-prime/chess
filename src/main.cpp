#include "square.hpp"
#include "piece.hpp"
#include "bitboard.hpp"
#include "move.hpp"
#include "node.hpp"

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

template <side_e side, bool divide>
std::size_t perft(const node &current, int depth) noexcept
{
    std::size_t count = 0;
    std::array<move_t, 256> buffer;
    auto moves = current.generate<side, node::all>(buffer);
    for (const auto &move : moves)
    {
        std::size_t count_ = 0;
        if (depth <= 1)
            count_ += 1;
        else
        {
            node succ(current);
            succ.execute<side>(move);
            std::array<move_t, 256> buffer2;
            count_ += depth == 2 ? succ.generate<~side, node::all>(buffer2).size() : perft<~side, false>(succ, depth - 1);
        }
        count += count_;
        if constexpr (divide) {
            std::println("{}{:16L}", move, count_);
        }
    }
    return count;
}

int main() {
    using as_floating_point = std::chrono::duration<double, std::ratio<1>>;

    side_e side = WHITE;
    // const node current {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"sv, side};
    const node current {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"sv, side};
    auto time0 = std::chrono::high_resolution_clock::now();
    std::size_t counter_ = perft<WHITE, true>(current, 6);
    auto time1 = std::chrono::high_resolution_clock::now();
    auto time = duration_cast<as_floating_point>(time1 - time0).count();
    std::println("{:7.3f} {:16L} {:16L}", time, counter_, size_t(counter_ / time));
}