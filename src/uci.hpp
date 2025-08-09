#pragma once

#include "position.hpp"
#include "evaluator.hpp"
#include "transposition.hpp"
#include "history.hpp"
#include "searcher.hpp"

#include <functional>
#include <iostream>
// #include <regex>
#include <map>
#include <print>
#include <sstream>

#include <chrono>

struct uci_interface {

    uci_interface()
    // : options {
    //     {"Hash", "1"}
    // }, 
    : commands {
        {"ucinewgame"sv, std::bind(&uci_interface::ucinewgame, this, std::placeholders::_1)},
        {"uci"sv, std::bind(&uci_interface::uci, this, std::placeholders::_1)},
        // {"setoption"sv, std::bind(&uci_interface::setoption, this, std::placeholders::_1)},
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
        // std::println("option name Hash type spin default 1 min 1 max 16");
        std::println("uciok");
    }

    // void setoption(std::string_view args) {
    //     static const std::regex re("setoption name (\\w+) value (\\w+)");
    //     if (std::cmatch m; std::regex_match(&*args.begin(), &*args.end(), m, re)) {
    //         options[m[1].str()] = m[2].str();
    //     }
    // }

    void isready(std::string_view) {
        // std::println("hash = {}", options["Hash"]);
        std::println("readyok");
    }

    void ucinewgame(std::string_view) {
        position_ = position_t::STARTPOS;
        searcher.clear();
    }

    void position(std::string_view args) {
        args.remove_prefix("position "sv.size());

        auto parts = std::views::split(args, " moves "sv);
        auto part = parts.begin();

        std::string_view position_part {*part++};
        if (position_part == "startpos"sv) {
            position_ = position_t::STARTPOS;
        } else {
            position_part.remove_prefix("fen "sv.size());
            position_ = position_part;
        }

        if (part != parts.end()) {
            std::string_view moves_part {*part++};
            for (auto&& move : std::views::split(moves_part, ' ')) {
                position_.make_move(std::string_view{move});
            }
        }
    }

    void go(std::string_view args) {
        args.remove_prefix("go "sv.size());

        auto parts = std::views::split(args, ' ');
        auto part = parts.begin();

        part++; //wtime
        std::string_view wtime_part {*part++};
        std::int64_t wtime;
        std::from_chars(&*wtime_part.begin(), &*wtime_part.end(), wtime);

        part++; //btime
        std::string_view btime_part {*part++};
        std::int64_t btime;
        std::from_chars(&*btime_part.begin(), &*btime_part.end(), btime);

        part++; //winc
        std::string_view winc_part {*part++};
        std::int64_t winc;
        std::from_chars(&*winc_part.begin(), &*winc_part.end(), winc);

        part++; //binc
        std::string_view binc_part {*part++};
        std::int64_t binc;
        std::from_chars(&*binc_part.begin(), &*binc_part.end(), binc);

        auto timeleft = (position_.get_side() == WHITE) ? wtime : btime;
        auto timeinc = (position_.get_side() == WHITE) ? winc : binc;
        auto timetogo = timeinc;

        if (part != parts.end()) {
            part++; //movestogo
            std::string_view movestogo_part {*part++};
            auto movestogo = std::stoll(movestogo_part.data()) + 1;
            timetogo += timeleft / movestogo;
        } else {
            timetogo += timeleft * 5 / 100;
        }

        search = std::jthread{[&] {
            constexpr auto depth = 100;
            move_t best = searcher(depth);
            std::println("bestmove {}", best);
            std::fflush(stdout);
        }};
        
        timer = std::jthread{[this, timetogo] {
            using clock = std::chrono::high_resolution_clock;
            auto end = clock::now() + std::chrono::milliseconds(timetogo);
            while (clock::now() < end) {
                if (searcher.should_stop()) {
                    break;
                }
                std::this_thread::sleep_for(1ms);
            }
            searcher.request_stop();
        }};
        
        if (timer.joinable())
            timer.join();

        searcher.clear();
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

    // std::map<std::string, std::string> options;
    std::map<std::string_view, std::function<void(std::string_view)>> commands;

    position_t position_;
    transposition_t transposition;
    killer_t killer;
    history_t history;
    evaluator evaluator;
    searcher_t searcher{position_, transposition, killer, history, evaluator};
    std::jthread search;
    std::jthread timer;
};
