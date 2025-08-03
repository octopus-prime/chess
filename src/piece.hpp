#pragma once

#include "side.hpp"

enum type_e : int8_t {
    NO_TYPE, 
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};
constexpr inline size_t TYPE_MAX = 8;

inline constexpr type_e operator""_t(const char* str, size_t len) {
    if (len == 0) return NO_TYPE;
    switch (str[0] | 32) {
        case 'p': return PAWN;
        case 'n': return KNIGHT;
        case 'b': return BISHOP;
        case 'r': return ROOK;
        case 'q': return QUEEN;
        case 'k': return KING;
        default: return NO_TYPE;
    }
}

template <>
struct std::formatter<type_e> {
    constexpr auto parse(std::format_parse_context& ctx){
        return ctx.begin();
    }

    auto format(type_e type, std::format_context& ctx) const {
        std::string_view ch;
        switch (type) {
            case NO_TYPE: ch = ""sv; break;
            case PAWN: ch = "p"sv; break;
            case KNIGHT: ch = "n"sv; break;
            case BISHOP: ch = "b"sv; break;
            case ROOK: ch = "r"sv; break;
            case QUEEN: ch = "q"sv; break;
            case KING: ch = "k"sv; break;
        }
        return std::format_to(ctx.out(), "{}", ch);
    }
};

enum piece_e : int8_t {
    NO_PIECE = NO_TYPE,
    WPAWN = PAWN, WKNIGHT, WBISHOP, WROOK, WQUEEN, WKING,
    BPAWN = PAWN + TYPE_MAX, BKNIGHT, BBISHOP, BROOK, BQUEEN, BKING
};
constexpr inline size_t PIECE_MAX = 16;

struct piece {
    constexpr piece() noexcept
        : value{NO_PIECE} {}

    constexpr piece(piece_e value) noexcept
        : value{value} {}

    constexpr piece(side_e side, type_e type) noexcept
        : piece(static_cast<piece_e>(TYPE_MAX * int{side} + int{type})) {}

    constexpr piece(std::string_view view) noexcept : value{NO_PIECE}  {
        if (view.empty()) return;
        side_e side = std::isupper(view[0]) ? WHITE : BLACK;
        type_e type  = operator""_t(view.data(), view.size());
        value = piece{side, type};
    }

    constexpr side_e side() const noexcept {
        return static_cast<side_e>(value / TYPE_MAX);
    }

    constexpr type_e type() const noexcept {
        return static_cast<type_e>(value % TYPE_MAX);
    }

    constexpr operator piece_e() const noexcept {
        return value;
    }

   private:
    piece_e value;
};

inline constexpr piece operator""_p(const char* str, size_t len) {
    return piece{std::string_view{str, len}};
}

constexpr std::array<int16_t, TYPE_MAX> type_values = {
    0,   // NO_TYPE
    100, // PAWN
    300, // KNIGHT
    300, // BISHOP
    500, // ROOK
    900, // QUEEN
    10000    // KING
};

inline constexpr int16_t operator""_v(const char* str, size_t len) {
    return type_values[operator""_t(str, len)];
}
