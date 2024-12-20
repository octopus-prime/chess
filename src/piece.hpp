#pragma once

#include "side.hpp"

enum type_e : int8_t {
    NO_TYPE, 
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};
constexpr inline size_t TYPE_MAX = 8;

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
