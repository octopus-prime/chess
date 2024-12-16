#pragma once

#include <nnue/common.hpp>

namespace nnue {

enum {
    WHITE,
    BLACK,
    COLOR_NB = 2
};

enum : int {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE,

    SQUARE_ZERO = 0,
    SQUARE_NB   = 64
};

enum {
    NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    ALL_PIECES = 0,
    PIECE_TYPE_NB = 8
};

enum {
    NO_PIECE,
    W_PAWN = PAWN,     W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = PAWN + 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};

enum {
    PS_NONE     = 0,
    PS_W_PAWN   = 0,
    PS_B_PAWN   = 1 * SQUARE_NB,
    PS_W_KNIGHT = 2 * SQUARE_NB,
    PS_B_KNIGHT = 3 * SQUARE_NB,
    PS_W_BISHOP = 4 * SQUARE_NB,
    PS_B_BISHOP = 5 * SQUARE_NB,
    PS_W_ROOK   = 6 * SQUARE_NB,
    PS_B_ROOK   = 7 * SQUARE_NB,
    PS_W_QUEEN  = 8 * SQUARE_NB,
    PS_B_QUEEN  = 9 * SQUARE_NB,
    PS_KING     = 10 * SQUARE_NB,
    PS_NB       = 11 * SQUARE_NB
};

constexpr std::uint16_t PieceSquareIndex[2][16] = {
    {PS_NONE, PS_W_PAWN, PS_W_KNIGHT, PS_W_BISHOP, PS_W_ROOK, PS_W_QUEEN, PS_KING, PS_NONE, PS_NONE, PS_B_PAWN, PS_B_KNIGHT, PS_B_BISHOP, PS_B_ROOK, PS_B_QUEEN, PS_KING, PS_NONE},
    {PS_NONE, PS_B_PAWN, PS_B_KNIGHT, PS_B_BISHOP, PS_B_ROOK, PS_B_QUEEN, PS_KING, PS_NONE, PS_NONE, PS_W_PAWN, PS_W_KNIGHT, PS_W_BISHOP, PS_W_ROOK, PS_W_QUEEN, PS_KING, PS_NONE}
};

constexpr std::uint16_t OrientTBL[2][SQUARE_NB] = {
    { SQ_H1, SQ_H1, SQ_H1, SQ_H1, SQ_A1, SQ_A1, SQ_A1, SQ_A1,
    SQ_H1, SQ_H1, SQ_H1, SQ_H1, SQ_A1, SQ_A1, SQ_A1, SQ_A1,
    SQ_H1, SQ_H1, SQ_H1, SQ_H1, SQ_A1, SQ_A1, SQ_A1, SQ_A1,
    SQ_H1, SQ_H1, SQ_H1, SQ_H1, SQ_A1, SQ_A1, SQ_A1, SQ_A1,
    SQ_H1, SQ_H1, SQ_H1, SQ_H1, SQ_A1, SQ_A1, SQ_A1, SQ_A1,
    SQ_H1, SQ_H1, SQ_H1, SQ_H1, SQ_A1, SQ_A1, SQ_A1, SQ_A1,
    SQ_H1, SQ_H1, SQ_H1, SQ_H1, SQ_A1, SQ_A1, SQ_A1, SQ_A1,
    SQ_H1, SQ_H1, SQ_H1, SQ_H1, SQ_A1, SQ_A1, SQ_A1, SQ_A1 },
    { SQ_H8, SQ_H8, SQ_H8, SQ_H8, SQ_A8, SQ_A8, SQ_A8, SQ_A8,
    SQ_H8, SQ_H8, SQ_H8, SQ_H8, SQ_A8, SQ_A8, SQ_A8, SQ_A8,
    SQ_H8, SQ_H8, SQ_H8, SQ_H8, SQ_A8, SQ_A8, SQ_A8, SQ_A8,
    SQ_H8, SQ_H8, SQ_H8, SQ_H8, SQ_A8, SQ_A8, SQ_A8, SQ_A8,
    SQ_H8, SQ_H8, SQ_H8, SQ_H8, SQ_A8, SQ_A8, SQ_A8, SQ_A8,
    SQ_H8, SQ_H8, SQ_H8, SQ_H8, SQ_A8, SQ_A8, SQ_A8, SQ_A8,
    SQ_H8, SQ_H8, SQ_H8, SQ_H8, SQ_A8, SQ_A8, SQ_A8, SQ_A8,
    SQ_H8, SQ_H8, SQ_H8, SQ_H8, SQ_A8, SQ_A8, SQ_A8, SQ_A8 }
};

#define B(v) (v * PS_NB)
constexpr std::uint16_t KingBuckets[2][SQUARE_NB] = {
    { B(28), B(29), B(30), B(31), B(31), B(30), B(29), B(28),
    B(24), B(25), B(26), B(27), B(27), B(26), B(25), B(24),
    B(20), B(21), B(22), B(23), B(23), B(22), B(21), B(20),
    B(16), B(17), B(18), B(19), B(19), B(18), B(17), B(16),
    B(12), B(13), B(14), B(15), B(15), B(14), B(13), B(12),
    B( 8), B( 9), B(10), B(11), B(11), B(10), B( 9), B( 8),
    B( 4), B( 5), B( 6), B( 7), B( 7), B( 6), B( 5), B( 4),
    B( 0), B( 1), B( 2), B( 3), B( 3), B( 2), B( 1), B( 0) },
    { B( 0), B( 1), B( 2), B( 3), B( 3), B( 2), B( 1), B( 0),
    B( 4), B( 5), B( 6), B( 7), B( 7), B( 6), B( 5), B( 4),
    B( 8), B( 9), B(10), B(11), B(11), B(10), B( 9), B( 8),
    B(12), B(13), B(14), B(15), B(15), B(14), B(13), B(12),
    B(16), B(17), B(18), B(19), B(19), B(18), B(17), B(16),
    B(20), B(21), B(22), B(23), B(23), B(22), B(21), B(20),
    B(24), B(25), B(26), B(27), B(27), B(26), B(25), B(24),
    B(28), B(29), B(30), B(31), B(31), B(30), B(29), B(28) }
};
#undef B

template<int Perspective>
constexpr std::uint16_t make_index(const std::uint16_t king_square, const std::uint16_t piece_square, const std::uint16_t piece_type) noexcept {
    return (piece_square ^ OrientTBL[Perspective][king_square]) + PieceSquareIndex[Perspective][piece_type] + KingBuckets[Perspective][king_square];
}

}  // namespace nnue
