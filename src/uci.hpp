#pragma once

#include "position.hpp"
#include "evaluator.hpp"
#include "transposition.hpp"
#include "history.hpp"
#include "searcher.hpp"

#include <functional>
#include <iostream>
#include <regex>
#include <map>
#include <print>
#include <sstream>

#include <chrono>

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
        position_ = position_t{};
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
            position_ = position_t{}; // fix fen
            if (m[2].matched) {
                for (auto&& move : std::views::split(m[2].str(), ' ')) {
                    std::string_view v {move};
                    move2_t move2{v};
                    position_.make_move(move2);
                }
            }
        }
    }

    // void go(std::string_view args) {
    //     static const std::regex re("go wtime (\\d*) btime (\\d*) winc (\\d*) binc (\\d*)( movestogo (\\d*))?");
    //     if (std::cmatch m; std::regex_match(&*args.begin(), &*args.end(), m, re)) {
    //         auto timeleft = std::stoll(m[position_.get_side() == WHITE ? 1 : 2].str());
    //         auto timeinc = std::stoll(m[position_.get_side() == WHITE ? 3 : 4].str());
    //         long timetogo = timeinc;
    //         if (m[6].matched) {
    //             auto movestogo = std::stoll(m[6].str()) + 1;
    //             timetogo += timeleft / movestogo;
    //         } else
    //             timetogo += timeleft * 5 / 100;
    //         searcher.clean();
    //         search = std::jthread{[&] {
    //             constexpr auto depth = 100;
    //             auto [score, pv] = searcher(depth);
    //             std::println("bestmove {}", pv.front());
    //             std::fflush(stdout);
    //         }};
    //         // searcher.stop_token = search.get_stop_token();
    //         timer = std::jthread{[this, timetogo] {
    //             std::this_thread::sleep_for(timetogo * 1ms);
    //             // if (!search.get_stop_token().stop_requested())
    //                 stop(""sv);
    //         }};
    //         if (timer.joinable())
    //             timer.join();
    //     }
    // }

    // void stop(std::string_view) {
    //     search.request_stop();
    //     if (search.joinable())
    //         search.join();
    // }

    void go(std::string_view args) {
        static const std::regex re("go wtime (\\d*) btime (\\d*) winc (\\d*) binc (\\d*)( movestogo (\\d*))?");
        if (std::cmatch m; std::regex_match(&*args.begin(), &*args.end(), m, re)) {
            auto timeleft = std::stoll(m[position_.get_side() == WHITE ? 1 : 2].str());
            auto timeinc = std::stoll(m[position_.get_side() == WHITE ? 3 : 4].str());
            long timetogo = timeinc;
            if (m[6].matched) {
                auto movestogo = std::stoll(m[6].str()) + 1;
                timetogo += timeleft / movestogo;
            } else
                timetogo += timeleft * 5 / 100;

            searcher.clean();
            
            search = std::jthread{[&] {
                constexpr auto depth = 100;
                move2_t best = searcher(depth);
                std::println("bestmove {}", best);
                std::fflush(stdout);
            }};
            
            timer = std::jthread{[this, timetogo] {
                std::this_thread::sleep_for(timetogo * 1ms);
                searcher.request_stop();
            }};
            
            if (timer.joinable())
                timer.join();
        }
    }

void stop(std::string_view) {
    searcher.request_stop();
    if (search.joinable())
        search.join();
}

    void quit(std::string_view args) {
        stop(args);
        std::exit(0);
    }

    std::map<std::string, std::string> options;
    std::map<std::string_view, std::function<void(std::string_view)>> commands;

    position_t position_;
    transposition2_t transposition;
    history2_t history;
    evaluator2 evaluator;
    searcher_t searcher{position_, transposition, history, evaluator};
    std::array<move2_t, position_t::MAX_MOVES_PER_GAME> pv_buffer;
    std::jthread search;
    std::jthread timer;
};
