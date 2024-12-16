#include "square.hpp"
#include "piece.hpp"
#include "bitboard.hpp"

#include <functional>
#include <iostream>
#include <regex>
#include <map>
#include <print>

struct uci_interface {

    uci_interface()
    : options {
        {"Hash", "1"}
    }, 
    commands {
        {"uci"sv, std::bind(&uci_interface::uci, this, std::placeholders::_1)},
        {"setoption"sv, std::bind(&uci_interface::setoption, this, std::placeholders::_1)},
        {"isready"sv, std::bind(&uci_interface::isready, this, std::placeholders::_1)},
        {"ucinewgame"sv, [](std::string_view) {}},
        {"position"sv, [](std::string_view args) {}},
        {"go"sv, [](std::string_view args) {}},
        {"quit"sv, [](std::string_view) { std::exit(0); }},
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
        std::cmatch m;
        if (std::regex_match(args.begin(), args.end(), m, re)) {
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
    }

    std::map<std::string, std::string> options;
    std::map<std::string_view, std::function<void(std::string_view)>> commands;
};

int main() {
    uci_interface uci;
    std::stringstream ss;
    ss << "uci\n";
    ss << "setoption name Hash value 16\n";
    ss << "isready\n";
    ss << "quit\n";
    uci.run(ss);
}