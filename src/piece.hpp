#pragma once

#include "side.hpp"

enum type_e : int8_t {
    no_type, 
    pawn, knight, bishop, rook, queen, king
};
constexpr inline size_t type_max = 8;

enum piece_e : int8_t {
    no_piece = no_type,
    wpawn = pawn, wknight, wbishop, wrook, wqueen, wking,
    bpawn = pawn + type_max, bknight, bbishop, brook, bqueen, bking
};
constexpr inline size_t piece_max = 16;

struct piece {
    constexpr piece() noexcept
        : value{no_piece} {}

    constexpr piece(piece_e value) noexcept
        : value{value} {}

    constexpr piece(side_e side, type_e type) noexcept
        : piece(static_cast<piece_e>(type_max * int{side} + int{type})) {}

    constexpr side_e side() const noexcept {
        return static_cast<side_e>(value / type_max);
    }

    constexpr type_e type() const noexcept {
        return static_cast<type_e>(value % type_max);
    }

    constexpr operator piece_e() const noexcept {
        return value;
    }

   private:
    piece_e value;
};
