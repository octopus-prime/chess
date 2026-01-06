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
        // searcher.clear();
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
        // move_picker_t::counter2 = 0; // Reset counter for each go command
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
            std::int64_t movestogo;
            std::from_chars(&*movestogo_part.begin(), &*movestogo_part.end(), movestogo);
            timetogo += timeleft / (movestogo + 1);
        } else {
            timetogo += timeleft * 4 / 100;
        }

        search = std::jthread{[=, this](std::stop_token stop_token) {
            auto end = Clock::now() + std::chrono::milliseconds(timetogo);
            auto should_stop = [=] () {
                return stop_token.stop_requested() || Clock::now() >= end;
            };
            searcher_t searcher{position_, transposition, history, evaluator, should_stop};
            // move_t best = 
            (void) searcher(100);

            // char buffer[20];
            // char* out = std::format_to(buffer, "bestmove {}\n", best);
            // std::fwrite(buffer, sizeof(char), out - buffer, stdout);
            // std::fflush(stdout);

            searcher.clear();
        }};

        search.detach();
    }

    void stop(std::string_view) {
        search.request_stop();
    }

    void quit(std::string_view args) {
        stop(args);
        std::exit(0);
    }

    // std::map<std::string, std::string> options;
    std::map<std::string_view, std::function<void(std::string_view)>> commands;

    position_t position_;
    transposition_t transposition;
    history_t history{position_};
    nnue::big_nnue big;
    nnue::small_nnue small;
    evaluator evaluator{big, small};
    std::jthread search;
};
